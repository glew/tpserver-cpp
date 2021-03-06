#ifndef GRAPH_H
#define GRAPH_H
/*  Graph class
 *
 *  Copyright (C) 2008  Ryan Neufeld and the Thousand Parsec Project

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

#include <planet.h>
#include <set>
namespace RiskRuleset {

class Graph {
public:
   Graph();
   ~Graph();
   bool addPlanet(IGObject::Ptr planet);
   bool addEdge(const uint32_t& id1, const uint32_t& id2);
   bool addEdge(IGObject::Ptr planet1, IGObject::Ptr planet2);
   std::set<uint32_t> getAdjacent(IGObject::Ptr planet);
   std::set<uint32_t> getAdjacent(uint32_t id);
private:
   struct Node {
      uint32_t id;
      std::set<Node*> adjacent;
      //I didn't think it pertinent to include a pointer to the object here
      //as that information is available through the object manager via ID
   };
   std::map<uint32_t,Node> nodeMap;
};

} //end namespace RiskRuleset

#endif
