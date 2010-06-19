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

    std::string addslashes(const std::string& in) const;
    uint32_t getTableVersion(const std::string& name);
private:
        void lock();
        void unlock();
        sqlite3* db;
        char* db_err;

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

