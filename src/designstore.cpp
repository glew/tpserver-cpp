/*  DesignStore class
 *
 *  Copyright (C) 2005  Lee Begg and the Thousand Parsec Project
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


#include "design.h"
#include "component.h"
#include "property.h"
#include "game.h"
#include "player.h"

#include "designstore.h"

unsigned int DesignStore::next_designid = 1;
unsigned int DesignStore::next_componentid = 1;
unsigned int DesignStore::next_propertyid = 1;

DesignStore::DesignStore(){
  
}

DesignStore::~DesignStore(){
  while(!designs.empty()){
    delete designs.begin()->second;
    designs.erase(designs.begin());
  }
  while(!components.empty()){
    delete components.begin()->second;
    components.erase(components.begin());
  }
  while(!properties.empty()){
    delete properties.begin()->second;
    properties.erase(properties.begin());
  }
}

Design* DesignStore::getDesign(unsigned int id){
  std::map<unsigned int, Design*>::iterator pos = designs.find(id);
  if(pos != designs.end()){
    return pos->second;
  }else{
    return NULL;
  }
}

Component* DesignStore::getComponent(unsigned int id){
  std::map<unsigned int, Component*>::iterator pos = components.find(id);
  if(pos != components.end()){
    return pos->second;
  }else{
    return NULL;
  }
}

Property* DesignStore::getProperty(unsigned int id){
  std::map<unsigned int, Property*>::iterator pos = properties.find(id);
  if(pos != properties.end()){
    return pos->second;
  }else{
    return NULL;
  }
}

void DesignStore::addDesign(Design* d){
  d->setDesignId(next_designid++);
  d->eval();
  designs[d->getDesignId()] = d;
  Player* player = Game::getGame()->getPlayer(d->getOwner());
  player->addVisibleDesign(d->getDesignId());
  if(d->isValid()){
    player->addUsableDesign(d->getDesignId());
  }
}

bool DesignStore::modifyDesign(Design* d){
  Design* current = designs[d->getDesignId()];
  if(current == NULL || current->getOwner() != d->getOwner() || current->getNumExist() != 0)
    return false;
  Player* player = Game::getGame()->getPlayer(d->getOwner());
  player->removeUsableDesign(d->getDesignId());
  d->eval();
  designs[d->getDesignId()] = d;
  if(d->isValid()){
    player->addUsableDesign(d->getDesignId());
  }
  delete current;
  return true;
}