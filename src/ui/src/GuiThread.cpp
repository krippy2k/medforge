#include "medforge/ui/GuiThread.h"

#include <QCoreApplication>
#include <QThread>

namespace medforge::ui {

bool onGuiThread() {
    const QCoreApplication* application = QCoreApplication::instance();
    return application != nullptr && QThread::currentThread() == application->thread();
}

} // namespace medforge::ui
