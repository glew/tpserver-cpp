#ifndef MOVE_H
#define MOVE_H
/*  Move Order
 *
 *  Copyright (C) 2004, 2007  Lee Begg and the Thousand Parsec Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <tpserver/order.h>
#include <tpserver/vector3d.h>
class SpaceCoordParam;

class Move : public Order{
  public:
    Move();
    virtual ~Move();

    Vector3d getDest() const;
    void setDest(const Vector3d & ndest);

    int getETA(IGObject::Ptr ob) const;

    void createFrame(OutputFrame::Ptr f, int pos);
    void inputFrame(InputFrame::Ptr f, uint32_t playerid);

    bool doOrder(IGObject::Ptr ob);

    Order* clone() const;

  private:
    SpaceCoordParam* coords;


};

#endif
