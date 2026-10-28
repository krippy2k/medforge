#include "medforge/ui/VolumeViewport.h"

#ifdef MEDFORGE_HAS_VTK
#include <vtkFixedPointRayCastImage.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderUtilities.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLShaderCache.h>
#include <vtkOpenGLState.h>
#include <vtkOpenGLVertexArrayObject.h>
#include <vtkRayCastImageDisplayHelper.h>
#include <vtkRenderer.h>
#include <vtkShaderProgram.h>
#include <vtkSmartPointer.h>
#include <vtkTextureObject.h>
#include <vtkVersionMacros.h>
#include <vtkVolume.h>
#include <vtk_glad.h>

#include <algorithm>

namespace {

class MedForgeRayCastImageDisplayHelper : public vtkRayCastImageDisplayHelper {
public:
    static MedForgeRayCastImageDisplayHelper* New();
    vtkTypeMacro(MedForgeRayCastImageDisplayHelper, vtkRayCastImageDisplayHelper);

    void RenderTexture(vtkVolume* volume,
                       vtkRenderer* renderer,
                       int imageMemorySize[2],
                       int imageViewportSize[2],
                       int imageInUseSize[2],
                       int imageOrigin[2],
                       float requestedDepth,
                       unsigned char* image) override;
    void RenderTexture(vtkVolume* volume,
                       vtkRenderer* renderer,
                       int imageMemorySize[2],
                       int imageViewportSize[2],
                       int imageInUseSize[2],
                       int imageOrigin[2],
                       float requestedDepth,
                       unsigned short* image) override;
    void RenderTexture(vtkVolume* volume,
                       vtkRenderer* renderer,
                       vtkFixedPointRayCastImage* image,
                       float requestedDepth) override;
    void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
    MedForgeRayCastImageDisplayHelper() = default;
    ~MedForgeRayCastImageDisplayHelper() override = default;

private:
    void blit(vtkRenderer* renderer,
              const int imageMemorySize[2],
              const int imageViewportSize[2],
              const int imageInUseSize[2],
              const int imageOrigin[2],
              const void* image,
              int vtkType,
              float sampleDistance);

    vtkSmartPointer<vtkTextureObject> texture_;
    vtkSmartPointer<vtkOpenGLVertexArrayObject> vao_;
};

vtkStandardNewMacro(MedForgeRayCastImageDisplayHelper);

void MedForgeRayCastImageDisplayHelper::RenderTexture(vtkVolume* volume,
                                                      vtkRenderer* renderer,
                                                      int imageMemorySize[2],
                                                      int imageViewportSize[2],
                                                      int imageInUseSize[2],
                                                      int imageOrigin[2],
                                                      float requestedDepth,
                                                      unsigned char* image) {
    (void)volume;
    (void)requestedDepth;
    blit(renderer, imageMemorySize, imageViewportSize, imageInUseSize, imageOrigin, image,
         VTK_UNSIGNED_CHAR, 1.0f);
}

void MedForgeRayCastImageDisplayHelper::RenderTexture(vtkVolume* volume,
                                                      vtkRenderer* renderer,
                                                      int imageMemorySize[2],
                                                      int imageViewportSize[2],
                                                      int imageInUseSize[2],
                                                      int imageOrigin[2],
                                                      float requestedDepth,
                                                      unsigned short* image) {
    (void)volume;
    (void)requestedDepth;
    blit(renderer, imageMemorySize, imageViewportSize, imageInUseSize, imageOrigin, image,
         VTK_UNSIGNED_SHORT, 1.0f);
}

void MedForgeRayCastImageDisplayHelper::RenderTexture(vtkVolume* volume,
                                                      vtkRenderer* renderer,
                                                      vtkFixedPointRayCastImage* image,
                                                      float requestedDepth) {
    (void)volume;
    (void)requestedDepth;
    if (image == nullptr) {
        return;
    }
    const float sample = image->GetImageSampleDistance() > 0.0f ? image->GetImageSampleDistance()
                                                                 : 1.0f;
    blit(renderer, image->GetImageMemorySize(), image->GetImageViewportSize(),
         image->GetImageInUseSize(), image->GetImageOrigin(), image->GetImage(), VTK_UNSIGNED_SHORT,
         sample);
}

void MedForgeRayCastImageDisplayHelper::ReleaseGraphicsResources(vtkWindow* window) {
    if (texture_ != nullptr) {
        texture_->ReleaseGraphicsResources(window);
        texture_ = nullptr;
    }
    vao_ = nullptr;
}

void MedForgeRayCastImageDisplayHelper::blit(vtkRenderer* renderer,
                                             const int imageMemorySize[2],
                                             const int imageViewportSize[2],
                                             const int imageInUseSize[2],
                                             const int imageOrigin[2],
                                             const void* image,
                                             int vtkType,
                                             float sampleDistance) {
    if (renderer == nullptr || image == nullptr || imageMemorySize[0] <= 0 ||
        imageMemorySize[1] <= 0 || imageInUseSize[0] <= 0 || imageInUseSize[1] <= 0 ||
        imageViewportSize[0] <= 0 || imageViewportSize[1] <= 0) {
        return;
    }
    auto* window = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());
    if (window == nullptr || window->GetShaderCache() == nullptr || window->GetState() == nullptr) {
        return;
    }
    if (texture_ == nullptr) {
        texture_ = vtkSmartPointer<vtkTextureObject>::New();
    }
    texture_->SetContext(window);
    if (!texture_->Create2DFromRaw(static_cast<unsigned int>(imageMemorySize[0]),
                                   static_cast<unsigned int>(imageMemorySize[1]), 4, vtkType,
                                   const_cast<void*>(image))) {
        return;
    }

    static const char* vertex = R"(//VTK::System::Dec
in vec4 vertexMC;
in vec2 tcoordMC;
out vec2 tcoordVS;
void main() {
  tcoordVS = tcoordMC;
  gl_Position = vertexMC;
}
)";
    static const char* fragment = R"(//VTK::System::Dec
uniform sampler2D volumeImage;
uniform float pixelScale;
in vec2 tcoordVS;
//VTK::Output::Dec
void main() {
  gl_FragData[0] = texture(volumeImage, tcoordVS) * pixelScale;
}
)";
    vtkShaderProgram* program = window->GetShaderCache()->ReadyShaderProgram(vertex, fragment, nullptr);
    if (program == nullptr) {
        return;
    }

    const float scale = this->PixelScale > 0.0f ? this->PixelScale : 1.0f;
    texture_->Activate();
    program->SetUniformi("volumeImage", texture_->GetTextureUnit());
    program->SetUniformf("pixelScale", scale);

    const float drawWidth = static_cast<float>(imageInUseSize[0]) * sampleDistance;
    const float drawHeight = static_cast<float>(imageInUseSize[1]) * sampleDistance;
    const float width = static_cast<float>(imageViewportSize[0]);
    const float height = static_cast<float>(imageViewportSize[1]);
    const float x0 = (2.0f * static_cast<float>(imageOrigin[0]) / width) - 1.0f;
    const float y0 = (2.0f * static_cast<float>(imageOrigin[1]) / height) - 1.0f;
    const float x1 = (2.0f * (static_cast<float>(imageOrigin[0]) + drawWidth) / width) - 1.0f;
    const float y1 = (2.0f * (static_cast<float>(imageOrigin[1]) + drawHeight) / height) - 1.0f;
    const float tw = std::min(1.0f, static_cast<float>(imageInUseSize[0]) /
                                         static_cast<float>(imageMemorySize[0]));
    const float th = std::min(1.0f, static_cast<float>(imageInUseSize[1]) /
                                         static_cast<float>(imageMemorySize[1]));
    // VTK draws 0-1-2 and 0-2-3. Bottom-left, bottom-right, top-right, top-left
    // covers the image. The previous order left the right triangle undrawn.
    float vertices[12] = {x0, y0, 0.0f, x1, y0, 0.0f, x1, y1, 0.0f, x0, y1, 0.0f};
    float coordinates[8] = {0.0f, 0.0f, tw, 0.0f, tw, th, 0.0f, th};
    if (vao_ == nullptr) {
        vao_ = vtkSmartPointer<vtkOpenGLVertexArrayObject>::New();
    }

    vtkOpenGLState* state = window->GetState();
    state->vtkglDisable(GL_DEPTH_TEST);
    state->vtkglDisable(GL_CULL_FACE);
    state->vtkglEnable(GL_BLEND);
    if (this->PreMultipliedColors) {
        state->vtkglBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        state->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                      GL_ONE_MINUS_SRC_ALPHA);
    }
    vtkOpenGLRenderUtilities::RenderQuad(vertices, coordinates, program, vao_);
    texture_->Deactivate();
    state->vtkglDisable(GL_BLEND);
    state->vtkglEnable(GL_DEPTH_TEST);
}

class MedForgeVolumeObjectFactory : public vtkObjectFactory {
public:
    static MedForgeVolumeObjectFactory* New();
    vtkTypeMacro(MedForgeVolumeObjectFactory, vtkObjectFactory);
    const char* GetVTKSourceVersion() override {
        return VTK_SOURCE_VERSION;
    }
    const char* GetDescription() override {
        return "MedForge CPU volume display";
    }

protected:
    MedForgeVolumeObjectFactory() {
        this->RegisterOverride("vtkRayCastImageDisplayHelper", "MedForgeRayCastImageDisplayHelper",
                               "Blit the CPU ray-cast image", 1,
                               []() -> vtkObject* { return MedForgeRayCastImageDisplayHelper::New(); });
    }
    ~MedForgeVolumeObjectFactory() override = default;
};

vtkStandardNewMacro(MedForgeVolumeObjectFactory);

void registerFactoryOnce() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    vtkNew<MedForgeVolumeObjectFactory> factory;
    vtkObjectFactory::RegisterFactory(factory);
}

} // namespace
#endif

void registerVolumeRayCastHelper() {
#ifdef MEDFORGE_HAS_VTK
    registerFactoryOnce();
#endif
}
