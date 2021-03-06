#ifndef PERSISTENCE_H
#define PERSISTENCE_H
/*  Persistence class
 *
 *  Copyright (C) 2005, 2007, 2008  Lee Begg and the Thousand Parsec Project
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
#include <list>
#include <set>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <tpserver/protocolview.h>
#include <tpserver/protocolobject.h>

class IGObject;
class Order;
class OrderQueue;
class Board;
class Message;
class ResourceDescription;
class Player;
class Category;
class Design;
class Component;
class Property;
class ObjectView;
class DesignView;
class ComponentView;

class Persistence{
public:
    virtual ~Persistence();

    virtual bool init();
    virtual void shutdown();
    
    virtual bool saveGameInfo();
    virtual bool retrieveGameInfo();

    virtual bool saveObject(boost::shared_ptr<IGObject> ob);
    virtual boost::shared_ptr<IGObject> retrieveObject(uint32_t obid);
    virtual uint32_t getMaxObjectId();
    virtual IdSet getObjectIds();

    virtual bool saveOrderQueue(const boost::shared_ptr<OrderQueue> oq);
    virtual bool updateOrderQueue(const boost::shared_ptr<OrderQueue> oq);
    virtual boost::shared_ptr<OrderQueue> retrieveOrderQueue(uint32_t oqid);
    virtual bool removeOrderQueue(uint32_t oqid);
    virtual IdSet getOrderQueueIds();
    virtual uint32_t getMaxOrderQueueId();
    
    virtual bool saveOrder(uint32_t queueid, uint32_t ordid, Order* ord);
    virtual bool updateOrder(uint32_t queueid, uint32_t ordid, Order* ord);
    virtual Order* retrieveOrder(uint32_t queueid, uint32_t ordid);
    virtual bool removeOrder(uint32_t queueid, uint32_t ordid);
    

    virtual bool saveBoard(boost::shared_ptr< Board > board);
    virtual bool updateBoard(boost::shared_ptr< Board > board);
    virtual boost::shared_ptr< Board > retrieveBoard(uint32_t boardid);
    virtual uint32_t getMaxBoardId();
    virtual IdSet getBoardIds();

    virtual bool saveMessage(boost::shared_ptr< Message > msg);
    virtual boost::shared_ptr< Message > retrieveMessage(uint32_t msgid);
    virtual bool removeMessage(uint32_t msgid);
    virtual bool saveMessageList(uint32_t bid, std::list<uint32_t> list);
    virtual std::list<uint32_t> retrieveMessageList(uint32_t bid);
    virtual uint32_t getMaxMessageId();
    
    virtual bool saveResource(boost::shared_ptr<ResourceDescription> res);
    virtual boost::shared_ptr<ResourceDescription> retrieveResource(uint32_t restype);
    virtual uint32_t getMaxResourceId();
    virtual IdSet getResourceIds();

    virtual bool savePlayer(boost::shared_ptr<Player> player);
    virtual bool updatePlayer(boost::shared_ptr<Player> player);
    virtual boost::shared_ptr<Player> retrievePlayer(uint32_t playerid);
    virtual uint32_t getMaxPlayerId();
    virtual IdSet getPlayerIds();

    virtual bool saveCategory(boost::shared_ptr<Category> cat);
    virtual boost::shared_ptr<Category> retrieveCategory(uint32_t catid);
    virtual uint32_t getMaxCategoryId();
    virtual IdSet getCategoryIds();

    virtual bool saveDesign(boost::shared_ptr<Design> design);
    virtual bool updateDesign(boost::shared_ptr<Design> design);
    virtual boost::shared_ptr<Design> retrieveDesign(uint32_t designid);
    virtual uint32_t getMaxDesignId();
    virtual IdSet getDesignIds();

    virtual bool saveComponent(boost::shared_ptr<Component> comp);
    virtual boost::shared_ptr<Component> retrieveComponent(uint32_t compid);
    virtual uint32_t getMaxComponentId();
    virtual IdSet getComponentIds();

    virtual bool saveProperty(boost::shared_ptr<Property> prop);
    virtual boost::shared_ptr<Property> retrieveProperty(uint32_t propid);
    virtual uint32_t getMaxPropertyId();
    virtual IdSet getPropertyIds();
    
    virtual bool saveObjectView(uint32_t playerid, boost::shared_ptr<ObjectView>);
    virtual boost::shared_ptr<ObjectView> retrieveObjectView(uint32_t playerid, uint32_t objectid, uint32_t turn = 0xffffffff);
    
    virtual bool saveDesignView(uint32_t playerid, boost::shared_ptr<DesignView>);
    virtual boost::shared_ptr<DesignView> retrieveDesignView(uint32_t playerid, uint32_t designid);
    
    virtual bool saveComponentView(uint32_t playerid, boost::shared_ptr<ComponentView>);
    virtual boost::shared_ptr<ComponentView> retrieveComponentView(uint32_t playerid, uint32_t componentid);

    bool saveProtocolView(uint32_t playerid, ProtocolView::Ptr view);
    ProtocolView::Ptr retrieveProtocolView(FrameType viewtype, uint32_t playerid, uint32_t objectid);

    bool saveProtocolObject(ProtocolObject::Ptr object);
    ProtocolObject::Ptr retrieveProtocolObject(FrameType objtype, uint32_t id);
    uint32_t getMaxProtocolObjectId( FrameType objtype );
    IdSet getProtocolObjectIds( FrameType objtype );
};

#endif
