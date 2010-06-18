/* 
 * File:   sqlitepersistence.cpp
 * Author: greg
 * 
 * Created on June 17, 2010, 10:54 AM
 */

#include <sstream>

#include <tpserver/logging.h>
#include <tpserver/settings.h>
#include <tpserver/game.h>
#include <tpserver/ordermanager.h>
#include <tpserver/objecttypemanager.h>

#include <tpserver/object.h>
#include <tpserver/orderqueue.h>
#include <tpserver/order.h>
#include <tpserver/orderparameter.h>
#include <tpserver/orderparameters.h>
#include <tpserver/board.h>
#include <tpserver/message.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/player.h>
#include <tpserver/playerview.h>
#include <tpserver/category.h>
#include <tpserver/design.h>
#include <tpserver/propertyvalue.h>
#include <tpserver/component.h>
#include <tpserver/property.h>

#include <tpserver/objectview.h>
#include <tpserver/designview.h>
#include <tpserver/componentview.h>

//Objectparameters
#include <tpserver/position3dobjectparam.h>
#include <tpserver/velocity3dobjectparam.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/resourcelistobjectparam.h>
#include <tpserver/referenceobjectparam.h>
#include <tpserver/refquantitylistobjectparam.h>
#include <tpserver/integerobjectparam.h>
#include <tpserver/sizeobjectparam.h>
#include <tpserver/mediaobjectparam.h>

#include <my_global.h>
#include <my_sys.h>
#include <sqlite3.h>

#include "sqlitepersistence.h"

// TODO:
//extern "C" ...

// TODO:
//class SqliteEception : public std::exception {



SqlitePersistence::SqlitePersistence() : conn(NULL) {
}

SqlitePersistence::~SqlitePersistence() {
    if(conn! = NULL)
        shutdown();
}

bool SqlitePersistence::init(){
    // CHECK: I think the only thing needed to open a connection to a SQLite db is the db name (location)
    
    lock();
    if(conn != NULL){
        Logger::getLogger()->warning("Persistence already running");
        unlock();
        return false;
    }
    Settings* conf = Settings::getSettings();
    
    const char* host = NULL;
    std::string shost = conf->get("sqlite_host");
    if(shost.length() != 0)
        host = shost.c_str();
    const char* user = NULL;
    std::string suser = conf->get("sqlite_user");
    if(suser.length() != 0)
        user = suser.c_str();
    const char* pass = NULL;
    std::string spass = conf->get("sqlite_pass");
    if(spass.length() != 0)
        pass = spass.c_str();
    const char* db = NULL;
    std::string sdb = conf->get("sqlite_db");
    if(sdb.length() != 0)
        db = sdb.c_str();
    else{
        Logger::getLogger()->error("sqlite database name not specified");
        // TODO: Look into conn and its relevence to sqlite from mysql
        //sqlite3_close(db) // but db was never opened
        conn = NULL;
        unlock();
        return false;
    }
    
    const char* sock = NULL;
    std::string ssock = conf->get("mysql_socket");
    if(ssock.length() != 0)
        sock = ssock.c_str();
    int port = atoi(conf->get("sqlite_port").c_str());
//    if(mysql_real_connect(conn, host, user, pass, db, port, sock, CLIENT_FOUND_ROWS) == NULL){
//        Logger::getLogger()->error("Could not connect to sql");
//        conn = NULL;
//        unlock();
//        return false;
//    }
    
    if (sqlite3_open(db, &db)){
        Loggger::getLogger()->error("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        unlock();
        return false;
    }
    
    // check for tables, create if necessary
    char* db_err;
    sqlite3_exec(db, "SELECT * FROM tableversion;", NULL, 0, &db_err);
    if(db_err != NULL){
        // create tables
        Logger::getLogger()->info("Creating database tables");
        try{
            sqlite3_exec(db, "CREATE TABLE tableversion ( tableid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                "name VARCHAR(50) NOT NULL UNIQUE, version INT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db, "INSERT INTO tableversion VALUES (NULL, 'tableversion', 1), (NULL, 'gameinfo', 1), "
            "(NULL, 'object', 0), (NULL, 'objectparamposition', 0), (NULL, 'objectparamvelocity', 0), "
            "(NULL, 'objectparamorderqueue', 0), (NULL, 'objectparamresourcelist', 0), "
            "(NULL, 'objectparamreference', 0), (NULL, 'objectparamrefquantitylist', 0), "
            "(NULL, 'objectparaminteger', 0), (NULL, 'objectparamsize', 0), (NULL, 'objectparammedia', 0), "
            "(NULL, 'orderqueue', 0), (NULL, 'orderqueueowner', 0), (NULL, 'orderqueueallowedtype', 0), "
            "(NULL, 'ordertype', 0), (NULL, 'orderresource', 0), (NULL, 'orderslot', 0), "
            "(NULL, 'orderparamspace', 0), (NULL, 'orderparamobject', 0), "
            "(NULL, 'orderparamstring', 0), (NULL, 'orderparamtime', 0), "
            "(NULL, 'orderparamlist', 0), "
            "(NULL, 'board', 0), (NULL, 'message', 0), (NULL, 'messagereference', 0), (NULL, 'messageslot', 0), "
            "(NULL, 'player', 0), (NULL, 'playerscore', 0), (NULL, 'playerdesignview', 0), (NULL, 'playerdesignviewcomp', 0), "
            "(NULL, 'playerdesignviewprop', 0), (NULL, 'playerdesignusable', 0), (NULL, 'playercomponentview', 0), "
            "(NULL, 'playercomponentviewcat', 0), (NULL, 'playercomponentviewproperty', 0), "
            "(NULL, 'playercomponentusable', 0), (NULL, 'playerobjectview', 0), (NULL, 'playerobjectowned', 0), "
            "(NULL, 'category', 0), (NULL, 'design',0), (NULL, 'designcomponent', 0), (NULL, 'designproperty', 0), "
            "(NULL, 'component', 0), (NULL, 'componentcat', 0), (NULL, 'componentproperty', 0), "
            "(NULL, 'property', 0), (NULL, 'propertycat', 0), (NULL, 'resourcedesc', 0);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE gameinfo (metakey VARCHAR(50) NOT NULL, ctime BIGINT UNSIGNED NOT NULL PRIMARY KEY, turnnum INT UNSIGNED NOT NULL, turnname VARCHAR(50) NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE object (objectid INT UNSIGNED NOT NULL, turnnum INT UNSIGNED NOT NULL, alive TINYINT UNSIGNED NOT NULL, type INT UNSIGNED NOT NULL, " 
                    "name TEXT NOT NULL, description TEXT NOT NULL, parentid INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turnnum));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamposition (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, posx BIGINT NOT NULL, posy BIGINT NOT NULL, posz BIGINT NOT NULL, relative INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamvelocity (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, velx BIGINT NOT NULL, vely BIGINT NOT NULL, velz BIGINT NOT NULL, relative INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamorderqueue (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, queueid INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamresourcelist (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, resid INT UNSIGNED NOT NULL, available INT UNSIGNED NOT NULL, possible INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos, resid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamreference (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, reftype INT NOT NULL, refval INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamrefquantitylist (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, reftype INT NOT NULL, refid INT UNSIGNED NOT NULL, quant INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos, reftype, refid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparaminteger (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, val INT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparamsize (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, size BIGINT UNSIGNED NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE objectparammedia (objectid INT UNSIGNED NOT NULL, turn INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, paramgroupid INT UNSIGNED NOT NULL, paramgrouppos INT UNSIGNED NOT NULL, url VARCHAR(200) NOT NULL, PRIMARY KEY(objectid, turn, playerid, paramgroupid, paramgrouppos));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueue (queueid INT UNSIGNED NOT NULL PRIMARY KEY, objectid INT UNSIGNED NOT NULL, active TINYINT NOT NULL, repeating TINYINT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueueowner (queueid INT UNSIGNED NOT NULL, playerid INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, playerid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderqueueallowedtype (queueid INT UNSIGNED NOT NULL, ordertype INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, ordertype));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE ordertype (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, type INT UNSIGNED NOT NULL, turns INT UNSIGNED NOT NULL, PRIMARY KEY(queueid, orderid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderresource (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, resourceid INT UNSIGNED NOT NULL, amount INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, resourceid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamspace (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, posx BIGINT NOT NULL, posy BIGINT NOT NULL, posz BIGINT NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamobject (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamstring (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, thestring TEXT NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamtime (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, turns INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderparamlist (queueid INT UNSIGNED NOT NULL, orderid INT UNSIGNED NOT NULL, position INT UNSIGNED NOT NULL, listid INT UNSIGNED NOT NULL, amount INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, orderid, position, listid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE orderslot (queueid INT UNSIGNED NOT NULL, slot INT UNSIGNED NOT NULL, "
                    "orderid INT UNSIGNED NOT NULL, PRIMARY KEY (queueid, slot, orderid), UNIQUE (queueid, slot), UNIQUE(queueid, orderid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE board (boardid INT UNSIGNED NOT NULL PRIMARY KEY, "
                    "name TEXT NOT NULL, description TEXT NOT NULL, nummessages INT UNSIGNED NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE message (messageid INT UNSIGNED NOT NULL PRIMARY KEY, subject TEXT NOT NULL, "
                    "body TEXT NOT NULL, turn INT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE messagereference (messageid INT UNSIGNED NOT NULL, type INT NOT NULL, "
                    "refid INT UNSIGNED NOT NULL, PRIMARY KEY (messageid, type, refid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE messageslot (boardid INT UNSIGNED NOT NULL, slot INT UNSIGNED NOT NULL, "
                    "messageid INT UNSIGNED NOT NULL UNIQUE, PRIMARY KEY (boardid, slot));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE player (playerid INT UNSIGNED NOT NULL PRIMARY KEY, name TEXT NOT NULL, "
                    "password TEXT NOT NULL, email TEXT NOT NULL, comment TEXT NOT NULL, boardid INT UNSIGNED NOT NULL, "
                    "alive TINYINT UNSIGNED NOT NULL, modtime BIGINT NOT NULL, UNIQUE (name(255)));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerscore (playerid INT UNSIGNED NOT NULL, scoreid INT UNSIGNED NOT NULL, scoreval INT UNSIGNED NOT NULL, PRIMARY KEY(playerid, scoreid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignview (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "completelyvisible TINYINT NOT NULL, namevisible TINYINT NOT NULL,"
                    "visname TEXT NOT NULL, descvisible TINYINT NOT NULL, visdesc TEXT NOT NULL, existvisible TINYINT NOT NULL, visexist INT UNSIGNED NOT NULL, ownervisible TINYINT NOT NULL, visowner INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, designid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignviewcomp (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "componentid INT UNSIGNED NOT NULL, quantity INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, designid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignviewprop (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "propertyid INT UNSIGNED NOT NULL, value TEXT NOT NULL, "
                    "PRIMARY KEY (playerid, designid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerdesignusable (playerid INT UNSIGNED NOT NULL, designid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, designid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentview (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "completelyvisible TINYINT NOT NULL, namevisible TINYINT NOT NULL, visname TEXT NOT NULL, "
                    "descvisible TINYINT NOT NULL, visdesc TEXT NOT NULL, reqfuncvis TINYINT NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentviewcat (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentviewproperty (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "propertyid INT UNSIGNED NOT NULL, PRIMARY KEY (playerid, componentid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playercomponentusable (playerid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerobjectview (playerid INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, "
                    "turnnum INT UNSIGNED NOT NULL, completelyvisible TINYINT NOT NULL, gone TINYINT NOT NULL, "
                    "namevisible TINYINT NOT NULL, visname TEXT NOT NULL, descvisible TINYINT NOT NULL, "
                    "visdesc TEXT NOT NULL, "
                    "modtime BIGINT UNSIGNED NOT NULL, PRIMARY KEY (playerid, objectid, turnnum));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE playerobjectowned (playerid INT UNSIGNED NOT NULL, objectid INT UNSIGNED NOT NULL, "
                    "PRIMARY KEY (playerid, objectid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE category (categoryid INT UNSIGNED NOT NULL PRIMARY KEY, name TEXT NOT NULL, "
                    "description TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE design (designid INT UNSIGNED NOT NULL PRIMARY KEY, categoryid INT UNSIGNED NOT NULL,"
                    "name TEXT NOT NULL, description TEXT NOT NULL, owner INT NOT NULL, inuse INT UNSIGNED NOT NULL,"
                    "numexist INT UNSIGNED NOT NULL, valid TINYINT NOT NULL, feedback TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE designcomponent (designid INT UNSIGNED NOT NULL, componentid INT UNSIGNED NOT NULL, "
                    "count INT UNSIGNED NOT NULL, PRIMARY KEY (designid, componentid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE designproperty (designid INT UNSIGNED NOT NULL, propertyid INT UNSIGNED NOT NULL, "
                    "value DOUBLE  NOT NULL, displaystring TEXT NOT NULL, PRIMARY KEY (designid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE component (componentid INT UNSIGNED NOT NULL PRIMARY KEY, "
                    "name TEXT NOT NULL, description TEXT NOT NULL, tpclrequiresfunc TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE componentcat (componentid INT UNSIGNED NOT NULL, categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (componentid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE componentproperty (componentid INT UNSIGNED NOT NULL, propertyid INT UNSIGNED NOT NULL, "
                    "tpclvaluefunc TEXT NOT NULL, PRIMARY KEY (componentid, propertyid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE property (propertyid INT UNSIGNED NOT NULL PRIMARY KEY,"
                    "rank INT UNSIGNED NOT NULL, name TEXT NOT NULL, displayname TEXT NOT NULL, description TEXT NOT NULL, "
                    "tpcldisplayfunc TEXT NOT NULL, tpclrequiresfunc TEXT NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE propertycat (propertyid INT UNSIGNED NOT NULL, categoryid INT UNSIGNED NOT NULL, PRIMARY KEY (propertyid, categoryid));", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
            sqlite3_exec(db,"CREATE TABLE resourcedesc (resourcetype INT UNSIGNED NOT NULL PRIMARY KEY, name_sig TEXT NOT NULL,"
                    "name_plur TEXT NOT NULL, unit_sig TEXT NOT NULL, uint_plur TEXT NOT NULL, description TEXT NOT NULL, "
                    "mass INT UNSIGNED NOT NULL, volume INT UNSIGNED NOT NULL, modtime BIGINT UNSIGNED NOT NULL);", NULL, 0, &db_err);
            if(db_err != NULL)
                throw std::exception();
        }catch(std::exception e){
            Logger::getLogger()->error("Sqlite creating tables: %s", db_err);
            Logger::getLogger()->error("You may need to delete the tables and start again");
            sqlite3_close(db);
            unlock();
            return false;
        }
    }else{
        // check for tables to be updates.
        
        //NOTE TO SELF: left off at line 370
    }
    
}

sqlitepersistence::sqlitepersistence(const sqlitepersistence& orig) {
}



