/*  Fleet object
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

#include <math.h>

#include <tpserver/frame.h>
#include <tpserver/order.h>
#include <tpserver/game.h>
#include "planet.h"
#include <tpserver/object.h>
#include <tpserver/objectmanager.h>
#include <tpserver/objectdatamanager.h>
#include <tpserver/design.h>
#include <tpserver/designstore.h>
#include <tpserver/ordermanager.h>

#include "fleet.h"

Fleet::Fleet():OwnedObject()
{
  damage = 0;
}

Fleet::~Fleet(){
}

void Fleet::addShips(int type, int number){
  ships[type] += number;
  touchModTime();
}

bool Fleet::removeShips(int type, int number){
  if(ships[type] >= number){
    ships[type] -= number;
    touchModTime();
    return true;
  }
  return false;
}

int Fleet::numShips(int type){
  return ships[type];
}

std::map<int, int> Fleet::getShips() const{
  return ships;
}

int Fleet::totalShips() const{
  int num = 0;
  for(std::map<int, int>::const_iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
    num += itcurr->second;
  }
  return num;
}

long long Fleet::maxSpeed(){
  double speed = 1e100;
  DesignStore* ds = Game::getGame()->getDesignStore();
  for(std::map<int, int>::iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
        speed = fmin(speed, ds->getDesign(itcurr->first)->getPropertyValue(ds->getPropertyByName("Speed")));
  }
  return (long long)(floor(speed));
}

int Fleet::firepower(bool draw){
  double fp = 0;
  DesignStore* ds = Game::getGame()->getDesignStore();
  for(std::map<int, int>::iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
    if(draw){
      fp += ds->getDesign(itcurr->first)->getPropertyValue(ds->getPropertyByName("WeaponDraw")) * itcurr->second;
    }else{
      fp += ds->getDesign(itcurr->first)->getPropertyValue(ds->getPropertyByName("WeaponWin")) * itcurr->second;
    }
  }
  return (int) (floor(fp));
}

bool Fleet::hit(int firepower){
    if(firepower != 0){
  damage += firepower;
  bool change = true;
  DesignStore* ds = Game::getGame()->getDesignStore();
  while(change){
    change = false;
    //find largest ship (by HP (prop 3))
    int shiptype = 0;
    int shiphp = 0;
    for(std::map<int, int>::iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
      Design *design = ds->getDesign(itcurr->first);
      if(shiphp < (int)design->getPropertyValue(ds->getPropertyByName("Armour"))){
	shiptype = itcurr->first;
        shiphp = (int)design->getPropertyValue(ds->getPropertyByName("Armour"));
      }
    }
    if(shiphp == 0){
        touchModTime();
      return false;
    }
    while(damage > shiphp && ships[shiptype] > 0){
      ships[shiptype]--;
      damage -= shiphp;
        Design* design = ds->getDesign(shiptype);
        design->removeDestroyed(1);
        ds->designCountsUpdated(design);
      change = true;
    }
    if(ships[shiptype] == 0){
      ships.erase(shiptype);
      change = true;
    }
    
  }
  touchModTime();
    }
  return true;
}

int Fleet::getDamage() const{
    return damage;
}

void Fleet::setDamage(int nd){
    damage = nd;
    touchModTime();
}

void Fleet::packExtraData(Frame * frame)
{
	OwnedObject::packExtraData(frame);
	
	frame->packInt(ships.size());
	for(std::map<int, int>::iterator itcurr = ships.begin(); itcurr != ships.end(); ++itcurr){
	  //if(itcurr->second > 0){
	    frame->packInt(itcurr->first);
	    frame->packInt(itcurr->second);
	    //}
	}
	
	frame->packInt(damage);

}

void Fleet::doOnceATurn(IGObject * obj)
{
  IGObject * pob = Game::getGame()->getObjectManager()->getObject(obj->getParent());
  if(pob->getType() == obT_Planet && ((Planet*)(pob->getObjectData()))->getOwner() == getOwner()){
    if(damage != 0){
        damage = 0;
        touchModTime();
    }
  }
    Game::getGame()->getObjectManager()->doneWithObject(obj->getParent());
}

void Fleet::packAllowedOrders(Frame * frame, int playerid){
  if(playerid == getOwner()){
    bool colonise = false;
    DesignStore* ds = Game::getGame()->getDesignStore();
    for(std::map<int, int>::iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
        if(ds->getDesign(itcurr->first)->getPropertyValue(ds->getPropertyByName("Colonise")) == 1.0){
	colonise = true;
	break;
      }
    }
    OrderManager * om = Game::getGame()->getOrderManager();
    if(colonise){
      frame->packInt(5);
      frame->packInt(om->getOrderTypeByName("Colonise"));
    }else{
      frame->packInt(4);
    }
    frame->packInt(om->getOrderTypeByName("Move"));
    frame->packInt(om->getOrderTypeByName("No Operation"));
    frame->packInt(om->getOrderTypeByName("SplitFleet"));
    frame->packInt(om->getOrderTypeByName("MergeFleet"));
    
  }else{
    frame->packInt(0);
  }
}

bool Fleet::checkAllowedOrder(int ot, int playerid){
  bool colonise = false;
  DesignStore* ds = Game::getGame()->getDesignStore();
  for(std::map<int, int>::iterator itcurr = ships.begin();
      itcurr != ships.end(); ++itcurr){
        if(ds->getDesign(itcurr->first)->getPropertyValue(ds->getPropertyByName("Colonise")) == 1.0){
      colonise = true;
      break;
    }
  }
  OrderManager * om = Game::getGame()->getOrderManager();
  return (playerid == getOwner() && (ot == om->getOrderTypeByName("Move") || ot == om->getOrderTypeByName("No Operation") || ot == om->getOrderTypeByName("SplitFleet") || ot == om->getOrderTypeByName("MergeFleet") || (colonise && ot == om->getOrderTypeByName("Colonise"))));
}

int Fleet::getContainerType(){
  return 0;
}

ObjectData* Fleet::clone(){
  return new Fleet();
}
