/******
 *
 *  rob's QToolTip class
 *
 *  Apparently Sharp concluded that ToolTips were not useful on the Zaurus and 
 * left them out of their Qtopia.  Unfortunately, QWhatsThis uses the 
 * QToolTips::palette(), that function returns all 0's, and that means that 
 * QWhatsThis windows come up with black background and black foreground.  By 
 * re-implementing this class, QWhatsThis calls this QToolTip::palette(), and 
 * gets a useful result.
 *
 *   Include this class in your own Zaurus application and QWhatsThis should work 
 * for you as well.
 * 
 *  The contents of this file are released without restriction to the public 
 *  domain.
 *
 *  Copyright (c) rob miller October, 2003
 *
 *****/

//#include "debug.h"
#include <qpalette.h>

class QToolTip {
 public:
  QToolTip() { rDebug("qtt constructor"); }
  ~QToolTip() { rDebug("qtt destructor"); }
  QPalette palette();
};

QPalette QToolTip::palette() {
  static bool init;
  static QPalette pt;

  if (! init) {  // only initialise once
    init=true;
    //rDebug("initialising my qtt-palette()");  //rDebug() is just qDebug() with a compile switch
    QColor fg = QColor(0x00,0x00,0x00);
    QColor bg = QColor(0xff,0xff,0xdc);

    pt.setColor(QColorGroup::Background,bg);
    pt.setBrush(QColorGroup::Foreground,fg);
  }

  return pt;
}
