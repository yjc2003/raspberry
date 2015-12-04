#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <stdarg.h>

#include "inventory.h"

using namespace std;

bool Inventory::createTableIfNotExist(std::string tablename, std::string createquery) {
	string query = "SELECT name FROM sqlite_master WHERE type='table' AND name = ?";
	if (getfirst(query.c_str(), 1, tablename.c_str()) != tablename) {
		cout << "Creating missing table '" << tablename << "'" << endl;
		getfirst(createquery.c_str());
		if (getfirst(query.c_str(), 1, tablename.c_str()) != tablename) {
			cerr << "Can't create table '" << tablename << "'" << endl;
			return false;
		}
	}
	return true;
}

Inventory::Inventory(const char *dbfile) {
	int rc = sqlite3_open(dbfile, &db);
	if( rc != SQLITE_OK ){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return;
	}
	/*
	CREATE TABLE rooms (uuid text, name text, location text);
	CREATE TABLE devices (uuid text, name text, room text);
	CREATE TABLE floorplans (uuid text, name text);
	CREATE TABLE devicesfloorplan (floorplan text, device text, x integer, y integer);

	*/
	createTableIfNotExist("devices","CREATE TABLE devices (uuid text, name text, room text)");
	createTableIfNotExist("rooms", "CREATE TABLE rooms (uuid text, name text, location text)");
	createTableIfNotExist("floorplans", "CREATE TABLE floorplans (uuid text, name text)");
	createTableIfNotExist("devicesfloorplan", "CREATE TABLE devicesfloorplan (floorplan text, device text, x integer, y integer)");
	createTableIfNotExist("locations", "CREATE TABLE locations (uuid text, name text, description text)");
	createTableIfNotExist("users", "CREATE TABLE users (uuid text, username text, password text, pin text, description text)");
}

string Inventory::getdevicename (string uuid) {
	string query = "select name from devices where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

string Inventory::getdeviceroom (string uuid) {
	string query = "select room from devices where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

int Inventory::setdevicename (string uuid, string name) {
	if (name == "") {
		string query = "delete from devices where uuid = ?";
		getfirst(query.c_str(), 1, uuid.c_str());
		return 0;
	}
        if (getdevicename(uuid) == "") { // does not exist, create
                string query = "insert into devices (name, uuid) VALUES (?, ?)";
                printf("creating device: %s\n", query.c_str());
                getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
        } else {
                string query = "update devices set name = ? where uuid = ?";
                getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
        }
        if (getdevicename(uuid) == name) {
                return 0;
        } else {
                return -1;
        }
} 

string Inventory::getroomname (string uuid) {
	string query = "select name from rooms where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

int Inventory::setroomname (string uuid, string name) { 
	if (getroomname(uuid) == "") { // does not exist, create
		string query = "insert into rooms (name, uuid) VALUES (?, ?)";
		printf("creating room: %s\n", query.c_str());
		getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
	} else {
		string query = "update rooms set name = ? where uuid = ?";
		getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
	}
	if (getroomname(uuid) == name) {
		return 0;
	} else {
		return -1;
	}
} 

int Inventory::setdeviceroom (string deviceuuid, string roomuuid) {
	string query = "update devices set room = ? where uuid = ?";
	getfirst(query.c_str(), 2, roomuuid.c_str(), deviceuuid.c_str());
	if (getdeviceroom(deviceuuid) == roomuuid) {
		return 0;
	} else {
		return -1;
	}
} 

string Inventory::getdeviceroomname (string uuid) {
	string query = "select room from devices where uuid = ?";
	return getroomname(getfirst(query.c_str(), 1, uuid.c_str()));
} 

Variant::Map Inventory::getrooms() {
	Variant::Map result;
	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(db, "select uuid, name, location from rooms", -1, &stmt, NULL);
	if(rc!=SQLITE_OK) {
                fprintf(stderr, "sql error #%d: %s\n", rc,sqlite3_errmsg(db));
                return result;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
		Variant::Map entry;
		const char *roomname = (const char*)sqlite3_column_text(stmt, 1);
		const char *location = (const char*)sqlite3_column_text(stmt, 2);
		const char *uuid = (const char*)sqlite3_column_text(stmt, 0);
		if (roomname != NULL) {
			entry["name"] = string(roomname);
		} else {
			entry["name"] = "";
		} 
		if (location != NULL) {
			entry["location"] = string(location);
		} else {	
			entry["location"] = "";
		}
		if (uuid != NULL) {
			result[uuid] = entry;
		}
	}

	sqlite3_finalize (stmt);

	return result;
} 
int Inventory::deleteroom (string uuid) {
	getfirst("BEGIN");
	string query = "update devices set room = '' where room = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	query = "delete from rooms where uuid = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	if (getroomname(uuid) != "") {
		getfirst("ROLLBACK");
		return -1;
	} else {
		getfirst("COMMIT");
		return 0;
	}
	getfirst("ROLLBACK");
	return -1;
}
string Inventory::getfirst(const char *query) {
    return getfirst(query, 0);
}
string Inventory::getfirst(const char *query, int n, ...) {
	sqlite3_stmt *stmt;
	int rc, i;
	string result;
    va_list args;

	rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	if(rc!=SQLITE_OK) {
		fprintf(stderr, "sql error #%d: %s\n", rc,sqlite3_errmsg(db));
		return result;
	}

    va_start(args, n);
    
    for(i = 0; i < n; i++) {
        sqlite3_bind_text(stmt, i + 1, va_arg(args, char*), -1, NULL);
    }

	rc = sqlite3_step(stmt);
	switch(rc) {
		case SQLITE_ERROR:
			fprintf(stderr, "step error: %s\n",sqlite3_errmsg(db));
			break;
		case SQLITE_ROW:
			if (sqlite3_column_type(stmt, 0) == SQLITE_TEXT) result =string( (const char*)sqlite3_column_text(stmt, 0));
			break;
	}

    va_end(args);
	sqlite3_finalize(stmt);

	return result;
}

string Inventory::getfloorplanname(std::string uuid) {
	string query = "select name from floorplans where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

int Inventory::setfloorplanname(std::string uuid, std::string name) {
	if (getfloorplanname(uuid) == "") { // does not exist, create
		string query = "insert into floorplans (name, uuid) VALUES (?, ?)";
		printf("creating floorplan: %s\n", query.c_str());
		getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
	} else {
		string query = "update floorplans set name = ? where uuid = ?";
		getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
	}
	if (getfloorplanname(uuid) == name) {
		return 0;
	} else {
		return -1;
	}
}

int Inventory::setdevicefloorplan(std::string deviceuuid, std::string floorplanuuid, int x, int y) {
	stringstream xstr, ystr;
	xstr << x;
	ystr << y;
	string query = "select floorplan from devicesfloorplan where floorplan = ? and device = ?";
	if (getfirst(query.c_str(), 2, floorplanuuid.c_str(), deviceuuid.c_str())==floorplanuuid) {
		// already exists, update
        query = "update devicesfloorplan set x=?, y=? where floorplan = ? and device = ?";
		getfirst(query.c_str(), 4, xstr.str().c_str(), ystr.str().c_str(), floorplanuuid.c_str(), deviceuuid.c_str());

	} else {
		// create new record
		query = "insert into devicesfloorplan (x, y, floorplan, device) VALUES (?, ?, ?, ?)";
		cout << query << endl;
		getfirst(query.c_str(), 4, xstr.str().c_str(), ystr.str().c_str(), floorplanuuid.c_str(), deviceuuid.c_str());
	}

	return 0;
}

int Inventory::deletefloorplan(std::string uuid) {
	getfirst("BEGIN");
	string query = "delete from devicesfloorplan where floorplan = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	query = "delete from floorplans where uuid = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	if (getfloorplanname(uuid) != "") {
	    getfirst("ROLLBACK");
		return -1;
	} else {
	    getfirst("COMMIT");
		return 0;
	}
	getfirst("ROLLBACK");
	return -1;
}

Variant::Map Inventory::getfloorplans() {
	Variant::Map result;
	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(db, "select uuid, name from floorplans", -1, &stmt, NULL);
	if(rc!=SQLITE_OK) {
                fprintf(stderr, "sql error #%d: %s\n", rc,sqlite3_errmsg(db));
                return result;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
		Variant::Map entry;
		const char *floorplanname = (const char*)sqlite3_column_text(stmt, 1);
		const char *uuid = (const char*)sqlite3_column_text(stmt, 0);
		if (floorplanname != NULL) {
			entry["name"] = string(floorplanname);
		} else {
			entry["name"] = "";
		} 

		// for each floorplan now fetch the device coordinates
		sqlite3_stmt *stmt2;
		int rc2;
		string query = "select device, x, y from devicesfloorplan where floorplan = ?";
		rc2 = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt2, NULL);
		if (rc2 != SQLITE_OK) {
			fprintf(stderr, "sql error #%d: %s\n", rc2,sqlite3_errmsg(db));
			continue;
		}
		sqlite3_bind_text(stmt2, 1, uuid, -1, NULL);
		while (sqlite3_step(stmt2) == SQLITE_ROW) {
			Variant::Map device;
			const char *deviceuuid = (const char*)sqlite3_column_text(stmt2, 0);
			const char *x = (const char*)sqlite3_column_text(stmt2, 1);
			const char *y = (const char*)sqlite3_column_text(stmt2, 2);
			device["x"] = atoi(x);
			device["y"] = atoi(y);
			entry[deviceuuid] = device;
		}

		sqlite3_finalize (stmt2);			

		if (uuid != NULL) {
			result[uuid] = entry;
		}
	}

	sqlite3_finalize (stmt);

	return result;
} 

string Inventory::getlocationname (string uuid) {
	string query = "select name from locations where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

string Inventory::getroomlocation(string uuid) {
	string query = "select location from rooms where uuid = ?";
	return getfirst(query.c_str(), 1, uuid.c_str());
}

int Inventory::setlocationname(string uuid, string name) {
        if (getlocationname(uuid) == "") { // does not exist, create
                string query = "insert into locations (name, uuid) VALUES (?, ?)";
                printf("creating location: %s\n", query.c_str());
                getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
        } else {
                string query = "update locations set name = ? where uuid = ?";
                getfirst(query.c_str(), 2, name.c_str(), uuid.c_str());
        }
        if (getlocationname(uuid) == name) {
                return 0;
        } else {
                return -1;
        }

}
int Inventory::setroomlocation(string roomuuid, string locationuuid) {
	string query = "update rooms set location = ? where uuid = ?";
	getfirst(query.c_str(), 2, locationuuid.c_str(), roomuuid.c_str());
	if (getroomlocation(roomuuid) == locationuuid) {
		return 0;
	}
	return -1;
}
int Inventory::deletelocation(string uuid) {
	string query = "update rooms set location = '' where location = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	query = "delete from locations where uuid = ?";
	getfirst(query.c_str(), 1, uuid.c_str());
	if (getlocationname(uuid) == "") {
		return 0;
	}
	return -1;
}
Variant::Map Inventory::getlocations() {
	Variant::Map result;
	sqlite3_stmt *stmt;
	int rc;

	rc = sqlite3_prepare_v2(db, "select uuid, name from locations", -1, &stmt, NULL);
	if(rc!=SQLITE_OK) {
                fprintf(stderr, "sql error #%d: %s\n", rc,sqlite3_errmsg(db));
                return result;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
		Variant::Map entry;
		const char *locationname = (const char*)sqlite3_column_text(stmt, 1);
		const char *uuid = (const char*)sqlite3_column_text(stmt, 0);
		if (locationname != NULL) {
			entry["name"] = string(locationname);
		} else {
			entry["name"] = "";
		} 
		if (uuid != NULL) {
			result[uuid] = entry;
		}
	}

	sqlite3_finalize (stmt);

	return result;
}

int Inventory::createuser(string uuid, string username, string password, string pin, string description) {
	return 0;
}
int Inventory::deleteuser(string uuid){
	return 0;
}
int Inventory::authuser(string uuid){
	return 0;
}
int Inventory::setpassword(string uuid){
	return 0;
}
int Inventory::setpin(string uuid){
	return 0;
}
int Inventory::setpermission(string uuid, string permission){
	return 0;
}
int Inventory::deletepermission(string uuid, string permission){
	return 0;
}
Variant::Map Inventory::getpermissions(string uuid){
	Variant::Map permissions;
	return permissions;
}


#ifdef INVENTORY_TEST
// gcc -DINVENTORY_TEST inventory.cpp -lqpidtypes -lsqlite3
int main(int argc, char **argv){
	Inventory inv("inventory.db");
	cout << inv.setdevicename("1234", "1235") << endl;
	cout << inv.deleteroom("1234") << endl;
	cout << inv.getdevicename("1234") << endl;
	cout << inv.getrooms() << endl;
	cout << inv.setfloorplanname("2235", "floorplan2") << endl;
	cout << inv.setdevicefloorplan("1234", "2235", 5, 2) << endl;
	cout << inv.getfloorplans() << endl;
	cout << inv.deletefloorplan("2235");
	cout << inv.getfloorplans() << endl;
}
#endif
