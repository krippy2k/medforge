#include "medforge/cad/Kernel.h"

#include "medforge_occt_resource.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

#if MEDFORGE_HAS_OCCT
#include <APIHeaderSection_MakeHeader.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <Poly_Triangulation.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#endif

namespace medforge::cad {

struct Shape::Impl {
#if MEDFORGE_HAS_OCCT
    TopoDS_Shape shape;
#endif
    bool valid = false;
    double volume = 0;
    core::Vec3 minimum{};
    core::Vec3 maximum{};
    geometry::TriangleMesh mesh;
    std::string message;
};

Shape adoptShape(std::shared_ptr<Shape::Impl> impl) {
    Shape shape;
    shape.impl_ = std::move(impl);
    return shape;
}

struct KernelAccess {
#if MEDFORGE_HAS_OCCT
    static const TopoDS_Shape& occ(const Shape& shape) { return shape.impl_->shape; }
#endif
};

namespace {

constexpr double kDegree = 0.017453292519943295;
constexpr double kOverlapVolumeMm3 = 1e-4;

void setError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

#if defined(_WIN32)
void setEnv(const char* name, const std::string& value) {
    _putenv_s(name, value.c_str());
}
#else
void setEnv(const char* name, const std::string& value) {
    setenv(name, value.c_str(), 1);
}
#endif

void ensureResources() {
    static const bool ready = [] {
        std::string root = MEDFORGE_OCCT_RESOURCE_DIR;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        if (const char* overrideRoot = std::getenv("MEDFORGE_OCCT_RESOURCES")) {
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
            if (overrideRoot[0] != '\0') {
                root = overrideRoot;
            }
        }
        if (root.empty()) {
            return false;
        }
        setEnv("CSF_STEPDefaults", root + "/XSTEPResource");
        setEnv("CSF_IGESDefaults", root + "/XSTEPResource");
        setEnv("CSF_XSMessage", root + "/XSMessage");
        setEnv("CSF_SHMessage", root + "/SHMessage");
        setEnv("CSF_PluginDefaults", root + "/StdResource");
        setEnv("CSF_StandardDefaults", root + "/StdResource");
        return true;
    }();
    (void)ready;
}

#if MEDFORGE_HAS_OCCT

struct Built {
    TopoDS_Shape shape;
    bool valid = false;
    double volume = 0;
    core::Vec3 minimum{};
    core::Vec3 maximum{};
    geometry::TriangleMesh mesh;
    std::string message;
};

double lengthOf(const core::Vec3& value) {
    return std::sqrt((value.x * value.x) + (value.y * value.y) + (value.z * value.z));
}

void measureSolid(Built& built) {
    if (built.shape.IsNull()) {
        built.message = "The result is empty";
        return;
    }
    BRepCheck_Analyzer analyzer(built.shape);
    int solids = 0;
    for (TopExp_Explorer explorer(built.shape, TopAbs_SOLID); explorer.More(); explorer.Next()) {
        ++solids;
    }
    GProp_GProps props;
    BRepGProp::VolumeProperties(built.shape, props);
    built.volume = props.Mass();
    Bnd_Box box;
    BRepBndLib::Add(built.shape, box, false);
    if (!box.IsVoid()) {
        double xmin = 0;
        double ymin = 0;
        double zmin = 0;
        double xmax = 0;
        double ymax = 0;
        double zmax = 0;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        built.minimum = {xmin, ymin, zmin};
        built.maximum = {xmax, ymax, zmax};
    }
    if (!analyzer.IsValid() || solids != 1 || built.volume <= 1e-6) {
        built.valid = false;
        built.volume = 0;
        built.message = "The result is not a watertight solid";
        return;
    }
    built.valid = true;
    BRepMesh_IncrementalMesh mesher(built.shape, 0.35);
    mesher.Perform();
    for (TopExp_Explorer faces(built.shape, TopAbs_FACE); faces.More(); faces.Next()) {
        const TopoDS_Face face = TopoDS::Face(faces.Current());
        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull() || triangulation->NbNodes() < 1) {
            continue;
        }
        const gp_Trsf transform = location.Transformation();
        const auto base = static_cast<std::uint32_t>(built.mesh.positions.size());
        built.mesh.positions.reserve(built.mesh.positions.size() +
                                     static_cast<std::size_t>(triangulation->NbNodes()));
        for (int node = 1; node <= triangulation->NbNodes(); ++node) {
            const gp_Pnt point = triangulation->Node(node).Transformed(transform);
            built.mesh.positions.push_back({point.X(), point.Y(), point.Z()});
        }
        const bool reversed = face.Orientation() == TopAbs_REVERSED;
        for (int triangle = 1; triangle <= triangulation->NbTriangles(); ++triangle) {
            int first = 0;
            int second = 0;
            int third = 0;
            triangulation->Triangle(triangle).Get(first, second, third);
            if (reversed) {
                std::swap(second, third);
            }
            built.mesh.indices.push_back(base + static_cast<std::uint32_t>(first - 1));
            built.mesh.indices.push_back(base + static_cast<std::uint32_t>(second - 1));
            built.mesh.indices.push_back(base + static_cast<std::uint32_t>(third - 1));
        }
    }
}

Shape fromBuilt(Built built, std::string* error) {
    auto impl = std::make_shared<Shape::Impl>();
    if (!built.valid) {
        const std::string message =
            built.message.empty() ? "The result is not a watertight solid" : built.message;
        impl->message = message;
        setError(error, message);
        return adoptShape(impl);
    }
    impl->shape = std::move(built.shape);
    impl->valid = true;
    impl->volume = built.volume;
    impl->minimum = built.minimum;
    impl->maximum = built.maximum;
    impl->mesh = std::move(built.mesh);
    return adoptShape(impl);
}

Shape fromOcc(const TopoDS_Shape& shape, std::string* error) {
    Built built;
    built.shape = shape;
    try {
        measureSolid(built);
    } catch (const Standard_Failure& failure) {
        built.valid = false;
        const char* text = failure.GetMessageString();
        built.message = std::string("Open CASCADE failed: ") + (text != nullptr ? text : "");
    }
    return fromBuilt(std::move(built), error);
}

void stampStep(const Handle(StepData_StepModel)& model) {
    Handle(TCollection_HAsciiString) text = new TCollection_HAsciiString(kResearchWatermark);
    APIHeaderSection_MakeHeader header(model);
    if (!header.IsDone()) {
        header.Init("medforge-prototype.step");
    }
    header.SetDescriptionValue(1, text);
    header.SetAuthorisation(text);
    header.SetOriginatingSystem(new TCollection_HAsciiString("MedForge"));
    header.Apply(model);
}

#endif

} // namespace

Shape::Shape() : impl_(std::make_shared<Impl>()) {}
Shape::~Shape() = default;
Shape::Shape(const Shape&) = default;
Shape& Shape::operator=(const Shape&) = default;
Shape::Shape(Shape&&) noexcept = default;
Shape& Shape::operator=(Shape&&) noexcept = default;

bool Shape::isNull() const {
#if MEDFORGE_HAS_OCCT
    return impl_ == nullptr || impl_->shape.IsNull();
#else
    return true;
#endif
}

bool Shape::isValidSolid() const {
    return impl_ != nullptr && impl_->valid;
}

double Shape::volumeMm3() const {
    return impl_ != nullptr ? impl_->volume : 0;
}

core::Vec3 Shape::boundsMin() const {
    return impl_ != nullptr ? impl_->minimum : core::Vec3{};
}

core::Vec3 Shape::boundsMax() const {
    return impl_ != nullptr ? impl_->maximum : core::Vec3{};
}

const geometry::TriangleMesh& Shape::mesh() const {
    static const geometry::TriangleMesh empty;
    return impl_ != nullptr ? impl_->mesh : empty;
}

const std::string& Shape::message() const {
    static const std::string empty;
    return impl_ != nullptr ? impl_->message : empty;
}

bool available() {
#if MEDFORGE_HAS_OCCT
    return true;
#else
    return false;
#endif
}

const char* kernelName() {
    return available() ? "Open CASCADE 7.9.3" : "unavailable";
}

Shape makeBox(const core::Vec3& corner, const core::Vec3& size, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (!(size.x > 0) || !(size.y > 0) || !(size.z > 0)) {
        setError(error, "Box size must be positive");
        return {};
    }
    try {
        BRepPrimAPI_MakeBox box(gp_Pnt(corner.x, corner.y, corner.z), size.x, size.y, size.z);
        const TopoDS_Shape shape = box.Shape();
        if (shape.IsNull()) {
            setError(error, "Box construction failed");
            return {};
        }
        return fromOcc(shape, error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("Box construction failed: ") + (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)corner;
    (void)size;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

Shape makeCylinder(const core::Vec3& origin,
                   const core::Vec3& axis,
                   double radiusMm,
                   double heightMm,
                   std::string* error) {
#if MEDFORGE_HAS_OCCT
    const double axisLength = lengthOf(axis);
    if (!(radiusMm > 0) || !(heightMm > 0) || !(axisLength > 1e-9)) {
        setError(error, "Cylinder radius, height, and axis must be positive");
        return {};
    }
    try {
        const gp_Ax2 axes(gp_Pnt(origin.x, origin.y, origin.z),
                          gp_Dir(axis.x / axisLength, axis.y / axisLength, axis.z / axisLength));
        BRepPrimAPI_MakeCylinder cylinder(axes, radiusMm, heightMm);
        const TopoDS_Shape shape = cylinder.Shape();
        if (shape.IsNull()) {
            setError(error, "Cylinder construction failed");
            return {};
        }
        return fromOcc(shape, error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("Cylinder construction failed: ") +
                            (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)origin;
    (void)axis;
    (void)radiusMm;
    (void)heightMm;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

Shape transformShape(const Shape& shape, const Pose& pose, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (shape.isNull()) {
        setError(error, "Cannot transform an empty solid");
        return {};
    }
    try {
        gp_Trsf aboutL;
        gp_Trsf aboutP;
        gp_Trsf aboutS;
        gp_Trsf translation;
        aboutL.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), pose.rotationDeg.x * kDegree);
        aboutP.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)), pose.rotationDeg.y * kDegree);
        aboutS.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), pose.rotationDeg.z * kDegree);
        translation.SetTranslation(
            gp_Vec(pose.translationMm.x, pose.translationMm.y, pose.translationMm.z));
        const gp_Trsf total =
            translation.Multiplied(aboutS).Multiplied(aboutP).Multiplied(aboutL);
        BRepBuilderAPI_Transform transformer(KernelAccess::occ(shape), total, Standard_True);
        if (!transformer.IsDone()) {
            setError(error, "Transform failed");
            return {};
        }
        return fromOcc(transformer.Shape(), error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("Transform failed: ") + (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)shape;
    (void)pose;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

Shape booleanCut(const Shape& target, const Shape& tool, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (target.isNull() || tool.isNull()) {
        setError(error, "Boolean input is empty");
        return {};
    }
    try {
        BRepAlgoAPI_Cut cut(KernelAccess::occ(target), KernelAccess::occ(tool));
        if (!cut.IsDone() || cut.HasErrors()) {
            setError(error, "Boolean failed");
            return {};
        }
        return fromOcc(cut.Shape(), error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("Boolean failed: ") + (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)target;
    (void)tool;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

Clearance measureClearance(const Shape& first, const Shape& second, double toleranceMm) {
    Clearance report;
    report.toleranceMm = toleranceMm;
    report.method = kClearanceMethod;
#if MEDFORGE_HAS_OCCT
    if (!(toleranceMm > 0)) {
        report.message = "Tolerance must be positive";
        return report;
    }
    if (first.isNull() || second.isNull()) {
        report.message = "Clearance needs two solids";
        return report;
    }
    try {
        BRepExtrema_DistShapeShape distance(KernelAccess::occ(first), KernelAccess::occ(second),
                                           toleranceMm);
        if (!distance.IsDone()) {
            report.message = "Distance failed";
            return report;
        }
        report.distanceMm = distance.Value();
        BRepAlgoAPI_Common common(KernelAccess::occ(first), KernelAccess::occ(second));
        if (common.IsDone() && !common.HasErrors()) {
            GProp_GProps props;
            BRepGProp::VolumeProperties(common.Shape(), props);
            report.commonVolumeMm3 = std::max(0.0, props.Mass());
        }
        report.overlap = report.commonVolumeMm3 > kOverlapVolumeMm3;
        report.ok = true;
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        report.message = std::string("Clearance failed: ") + (text != nullptr ? text : "");
    }
#else
    (void)first;
    (void)second;
    report.message = "Open CASCADE is not linked";
#endif
    return report;
}

Shape readStep(const std::string& path, std::string* error) {
#if MEDFORGE_HAS_OCCT
    ensureResources();
    try {
        STEPControl_Reader reader;
        if (reader.ReadFile(path.c_str()) != IFSelect_RetDone) {
            setError(error, "STEP file could not be read");
            return {};
        }
        if (reader.TransferRoots() < 1) {
            setError(error, "STEP file has no transferable solids");
            return {};
        }
        return fromOcc(reader.OneShape(), error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("STEP file could not be read: ") +
                            (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)path;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

Shape readIges(const std::string& path, std::string* error) {
#if MEDFORGE_HAS_OCCT
    ensureResources();
    try {
        IGESControl_Reader reader;
        if (reader.ReadFile(path.c_str()) != IFSelect_RetDone) {
            setError(error, "IGES file could not be read");
            return {};
        }
        if (reader.TransferRoots() < 1) {
            setError(error, "IGES file has no transferable solids");
            return {};
        }
        return fromOcc(reader.OneShape(), error);
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("IGES file could not be read: ") +
                            (text != nullptr ? text : ""));
        return {};
    }
#else
    (void)path;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

bool writeStep(const Shape& shape, const std::string& path, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (!shape.isValidSolid()) {
        setError(error, "STEP export requires a valid B-Rep solid");
        return false;
    }
    ensureResources();
    try {
        STEPControl_Writer writer;
        if (writer.Transfer(KernelAccess::occ(shape), STEPControl_AsIs) != IFSelect_RetDone) {
            setError(error, "STEP transfer failed");
            return false;
        }
        stampStep(writer.Model());
        if (writer.Write(path.c_str()) != IFSelect_RetDone) {
            setError(error, "STEP file could not be written");
            return false;
        }
        return true;
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("STEP file could not be written: ") +
                            (text != nullptr ? text : ""));
        return false;
    }
#else
    (void)shape;
    (void)path;
    setError(error, "Open CASCADE is not linked");
    return false;
#endif
}

bool writeIges(const Shape& shape, const std::string& path, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (!shape.isValidSolid()) {
        setError(error, "IGES export requires a valid B-Rep solid");
        return false;
    }
    ensureResources();
    try {
        IGESControl_Writer writer("MM", 1);
        if (!writer.AddShape(KernelAccess::occ(shape))) {
            setError(error, "IGES transfer failed");
            return false;
        }
        writer.ComputeModel();
        if (!writer.Write(path.c_str())) {
            setError(error, "IGES file could not be written");
            return false;
        }
        return true;
    } catch (const Standard_Failure& failure) {
        const char* text = failure.GetMessageString();
        setError(error, std::string("IGES file could not be written: ") +
                            (text != nullptr ? text : ""));
        return false;
    }
#else
    (void)shape;
    (void)path;
    setError(error, "Open CASCADE is not linked");
    return false;
#endif
}

Shape buildGuide(const GuideSpec& spec, std::string* error) {
#if MEDFORGE_HAS_OCCT
    if (!(spec.patchLengthMm > 0) || !(spec.patchWidthMm > 0) || !(spec.offsetMm > 0)) {
        setError(error, "Guide offset and contact patch must be positive");
        return {};
    }
    const double span = std::min(spec.patchLengthMm, spec.patchWidthMm);
    if (!(spec.holeDiameterMm > 0) || spec.holeDiameterMm >= span - 0.2) {
        setError(error, "Guide hole does not fit the contact patch");
        return {};
    }
    std::string local;
    const Shape blank = makeBox(spec.contactOriginMm,
                                {spec.patchLengthMm, spec.patchWidthMm, spec.offsetMm},
                                &local);
    if (blank.isNull()) {
        setError(error, local.empty() ? "Guide contact patch failed" : local);
        return {};
    }
    core::Vec3 holeOrigin = spec.contactOriginMm;
    holeOrigin.x += spec.patchLengthMm * 0.5;
    holeOrigin.y += spec.patchWidthMm * 0.5;
    holeOrigin.z -= 0.5;
    const Shape hole = makeCylinder(holeOrigin,
                                    {0, 0, 1},
                                    spec.holeDiameterMm * 0.5,
                                    spec.offsetMm + 1.0,
                                    &local);
    if (hole.isNull()) {
        setError(error, local.empty() ? "Guide hole failed" : local);
        return {};
    }
    return booleanCut(blank, hole, error);
#else
    (void)spec;
    setError(error, "Open CASCADE is not linked");
    return {};
#endif
}

} // namespace medforge::cad
