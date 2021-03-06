#ifndef BUILDFLEET_H
#define BUILDFLEET_H
/*  BuildFleet order
 *
 *  Copyright (C) 2004-2005, 2007  Lee Begg and the Thousand Parsec Project
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

#include <map>
#include <string>

#include <tpserver/order.h>

class ListParameter;
class StringParameter;

namespace MTSecRuleset {


class BuildFleet : public Order{
 public:
  BuildFleet();
  virtual ~BuildFleet();

  void createFrame(OutputFrame::Ptr f, int pos);
  void inputFrame(InputFrame::Ptr f, uint32_t playerid);

  bool doOrder(IGObject::Ptr ob);

  Order* clone() const;

 private:
  std::map<uint32_t, std::pair<std::string, uint32_t> > generateListOptions();
  bool removeResource(uint32_t restype, uint32_t amount);

  ListParameter * fleetlist;
  StringParameter * fleetname;
};

}
#endif

