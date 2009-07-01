#ifndef COMPONENT_H
#define COMPONENT_H
/*  Design Component class
 *
 *  Copyright (C) 2005, 2007  Lee Begg and the Thousand Parsec Project
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

#include <string>
#include <map>
#include <set>
#include <stdint.h>
#include <tpserver/modifiable.h>
#include <tpserver/describable.h>

class Component : public Modifiable, public Describable {
 public:
  Component();
  virtual ~Component();

  /// TODO: remove
  uint32_t getComponentId() const;
  std::set<uint32_t> getCategoryIds() const;
  bool isInCategory(uint32_t id) const;
  std::string getTpclRequirementsFunction() const;
  std::map<uint32_t, std::string> getPropertyList() const;

  /// TODO: remove
  void setComponentId(uint32_t id);
  void setCategoryIds(const std::set<uint32_t>& ids);
  void addCategoryId(uint32_t id);
  void setTpclRequirementsFunction(const std::string& a);
  void setPropertyList(std::map<uint32_t, std::string> pl);
  
  void setInUse(bool used=true);
  bool isInUse() const;
  void setParentDesignId(uint32_t designid);
  uint32_t getParentDesignId() const;
  
 protected:
  std::set<uint32_t> catids;
  std::string tpcl_requirements;
  std::map<uint32_t, std::string> propertylist;
  bool inuse;
  uint32_t parentdesignid;

};


#endif
