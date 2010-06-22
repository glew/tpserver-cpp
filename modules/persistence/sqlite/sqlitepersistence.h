/* 
 * File:   sqlitepersistence.h
 * Author: greg
 *
 * Created on June 17, 2010, 10:54 AM
 */

#ifndef _SQLITEPERSISTENCE_H
#define	_SQLITEPERSISTENCE_H

#include <string>
#include <map>

#include <tpserver/persistence.h>

typedef struct sqlite3 sqlite3;
class SpaceCoordParam;
class ObjectOrderParameter;
class ListParameter;
class StringParameter;
class TimeParameter;

class Position3dObjectParam;
class Velocity3dObjectParam;
class OrderQueueObjectParam;
class ResourceListObjectParam;
class ReferenceObjectParam;
class RefQuantityListObjectParam;
class IntegerObjectParam;
class SizeObjectParam;
class MediaObjectParam;

class SqlitePersistence : public Persistence{
public:
    SqlitePersistence();
    SqlitePersistence(const sqlitepersistence& orig);
    virtual ~SqlitePersistence();

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

    virtual bool saveBoard(boost::shared_ptr<Board> board);
    virtual bool updateBoard(boost::shared_ptr<Board> board);
    virtual boost::shared_ptr<Board> retrieveBoard(uint32_t boardid);
    virtual uint32_t getMaxBoardId();
    virtual IdSet getBoardIds();

    virtual bool saveMessage( boost::shared_ptr< Message > msg);
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

    virtual bool saveObjectView(uint32_t playerid, boost::shared_ptr<ObjectView> ov);
    virtual boost::shared_ptr<ObjectView> retrieveObjectView(uint32_t playerid, uint32_t objectid, uint32_t turn = 0xffffffff);

    virtual bool saveDesignView(uint32_t playerid, boost::shared_ptr<DesignView> dv);
    virtual boost::shared_ptr<DesignView> retrieveDesignView(uint32_t playerid, uint32_t designid);

    virtual bool saveComponentView(uint32_t playerid, boost::shared_ptr<ComponentView> cv);
    virtual boost::shared_ptr<ComponentView> retrieveComponentView(uint32_t playerid, uint32_t componentid);

    std::string addslashes(const std::string& in) const;
    uint32_t getTableVersion(const std::string& name);
private:

    bool updateSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos, SpaceCoordParam* scp);
    bool retrieveSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos, SpaceCoordParam* scp);
    bool removeSpaceCoordParam(uint32_t queueid, uint32_t ordid, uint32_t pos);
    bool updateListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ListParameter* lp);
    bool retrieveListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ListParameter* lp);
    bool removeListParameter(uint32_t queueid, uint32_t ordid, uint32_t pos);
    bool updateObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ObjectOrderParameter* ob);
    bool retrieveObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, ObjectOrderParameter* ob);
    bool removeObjectOrderParameter(uint32_t queueid, uint32_t ordid, uint32_t pos);
    bool updateStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, StringParameter* st);
    bool retrieveStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, StringParameter* st);
    bool removeStringParameter(uint32_t queueid, uint32_t ordid, uint32_t pos);
    bool updateTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, TimeParameter* tp);
    bool retrieveTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos, TimeParameter* tp);
    bool removeTimeParameter(uint32_t queueid, uint32_t ordid, uint32_t pos);
        void lock();
        void unlock();
        sqlite3* db;
        char* db_err;

    void singleQuery( const std::string& query );
    uint32_t valueQuery( const std::string& query );

};

class SqliteQuery {
  public:
    SqliteQuery( sqlite3* db, const std::string& new_query );
    ~SqliteQuery();

    static void lock() {}
    static void unlock() {}

    const std::string get( uint32_t index );
    int getInt( uint32_t index );
    uint64_t getU64( uint32_t index );
    bool validRow();
    bool nextRow();

  private:

    void fetchResult();
    sqlite3* database;
    char* db_err;
    sqlite3_stmt stmt;
    int result;
    bool row;

    std::string query;

    MysqlQuery() {};
};

#endif	/* _SQLITEPERSISTENCE_H */

