/*  colonise
 *
 *  Copyright (C) 2007  Tyler Shaub and the Thousand Parsec Project
 *  Copyright (C) 2008  Lee Begg and the Thousand Parsec Project
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

#include <tpserver/orderparameters.h>
#include <tpserver/message.h>
#include <tpserver/game.h>
#include <tpserver/object.h>
#include <tpserver/designstore.h>
#include <tpserver/objectmanager.h>
#include <tpserver/player.h>
#include <tpserver/playermanager.h>
#include <tpserver/playerview.h>

#include "fleet.h"
#include "planet.h"
#include "playerinfo.h"

#include "colonise.h"

namespace RFTS_ {

using std::string;
using std::set;

Colonise::Colonise() {
   name = "Colonise";
   description = "Colonise a planet";

   planet = (ObjectOrderParameter*)addOrderParameter( new ObjectOrderParameter("Planet", "The planet to colonise") );

   turns = 1;
}

Colonise::~Colonise() {

}

Order* Colonise::clone() const {
   Colonise *c = new Colonise();
   c->type = this->type;
   return c;
}

bool Colonise::doOrder(IGObject::Ptr obj) {
   
   Game *game = Game::getGame();
   ObjectManager *om = game->getObjectManager();
   
   Fleet *fleetData = dynamic_cast<Fleet*>(obj->getObjectBehaviour());

   IGObject::Ptr planetObj = om->getObject(planet->getObjectId());
   Planet *planetData = dynamic_cast<Planet*>(planetObj->getObjectBehaviour());

   Player::Ptr attacker = game->getPlayerManager()->getPlayer(fleetData->getOwner());
   Player::Ptr defender;
   
   Message::Ptr msg( new Message() );
   
   if(planetData == NULL || planetObj->getParent() != obj->getParent())
   {
      msg->setSubject("Colonise failed!");
		if( planetObj->getParent() != obj->getParent() )
			msg->setBody(string("Fleet, ") + obj->getName()  +" is not in this planet's system!");
		else
			msg->setBody("Tried to colonise a non-planet!");
      msg->addReference(rst_Action_Order, rsorav_Canceled);
   }
   else
   {
      uint32_t transId = PlayerInfo::getPlayerInfo(attacker->getID()).getTransportId();
      uint32_t colonists = fleetData->numShips(transId);
      fleetData->removeShips(transId, colonists);

      if(planetData->getOwner() != 0) // make sure the owner exists
         defender = game->getPlayerManager()->getPlayer(planetData->getOwner());
      string msgBody = string("Colonists from ") + attacker->getName() + "'s fleet " + obj->getName();

      if(fleetData->totalShips() == 0){
         obj->removeFromParent();
         om->scheduleRemoveObject(obj->getID());
         attacker->getPlayerView()->removeOwnedObject(obj->getID());
      }

      // colonists attack planet
      planetData->removeResource("Population", static_cast<uint32_t>(colonists * 4.f/3));

      // check for take over
      if(colonists >= planetData->getResource("Population").first / 2)
      {
         planetData->setOwner(attacker->getID());
         msgBody += string(" colonised ")  + planetObj->getName();
         PlayerInfo::getPlayerInfo(attacker->getID()).addVictoryPoints(VICTORY_POINTS_COLONISE);
      }
      else
      {
         msgBody += string(" attacked, but failed to colonise ") + planetObj->getName();
         PlayerInfo::getPlayerInfo(attacker->getID()).addVictoryPoints(VICTORY_POINTS_ATTACK);
      }

      msg->setBody(PlayerInfo::appAllVictoryPoints(msgBody));

      msg->setSubject("Colonise order complete");
      msg->addReference(rst_Action_Order, rsorav_Completion);
   }


   om->doneWithObject(planetObj->getID());

   msg->addReference(rst_Object, planetObj->getID());
   msg->addReference(rst_Object, obj->getID());
   
   attacker->postToBoard(msg);
   if(defender != NULL)
      defender->postToBoard( Message::Ptr( new Message(*msg)));
      

   return true;
}

}
