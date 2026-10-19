#include "spin_box_wheel_guard.hpp"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QEvent>
#include <QObject>
#include <QWidget>

namespace nebbie::qt {
namespace {

class SpinBoxWheelGuard final : public QObject {
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event == nullptr || event->type() != QEvent::Wheel) {
            return QObject::eventFilter(watched, event);
        }
        auto* spin = qobject_cast<QAbstractSpinBox*>(watched);
        if (spin == nullptr || spin->hasFocus()) {
            return QObject::eventFilter(watched, event);
        }
        // Let the surrounding scroll area receive the wheel instead of changing the value.
        if (QWidget* parent = spin->parentWidget()) {
            QApplication::sendEvent(parent, event);
        }
        return true;
    }
};

SpinBoxWheelGuard* shared_guard() {
    static SpinBoxWheelGuard guard;
    return &guard;
}

} // namespace

void installSpinBoxWheelGuard(QAbstractSpinBox* spin) {
    if (spin == nullptr) {
        return;
    }
    spin->setFocusPolicy(Qt::StrongFocus);
    spin->installEventFilter(shared_guard());
}

} // namespace nebbie::qt
