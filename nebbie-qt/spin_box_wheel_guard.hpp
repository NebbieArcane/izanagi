#pragma once

class QAbstractSpinBox;

namespace nebbie::qt {

/** Ignore mouse-wheel on spin boxes unless they already have keyboard focus.
 *  Prevents accidental value changes while scrolling a parent form. */
void installSpinBoxWheelGuard(QAbstractSpinBox* spin);

} // namespace nebbie::qt
