/*
     Copyright (C) 2012 Harald Klein <hari@vt100.at>

     This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License.
     This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

     See the GNU General Public License for more details.

     this is the core resolver component for ago control 
*/

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#ifndef __FreeBSD__
#include <sys/sysinfo.h>
#endif

#include <sstream>
#include <map>
#include <deque>

#include <uuid/uuid.h>

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Address.h>

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include "boost/filesystem.hpp"

#include "agoclient.h"
#include "version.h"

#ifndef VARIABLESMAPFILE
#define VARIABLESMAPFILE CONFDIR "/maps/variablesmap.json"
#endif

#ifndef DEVICESMAPFILE
#define DEVICESMAPFILE CONFDIR "/maps/devices.json"
#endif

#include "schema.h"
#include "inventory.h"

using namespace std;
using namespace qpid::messaging;
using namespace qpid::types;
using namespace agocontrol;

namespace fs = ::boost::filesystem;

AgoConnection *agoConnection;

Variant::Map inventory; // used to hold device registrations
Variant::Map schema;  
Variant::Map systeminfo; // holds system information
Variant::Map variables; // holds global variables
Variant::Map environment; // holds global environment like position, weather conditions, ..

Inventory *inv;
unsigned int discoverdelay;
bool persistence = false;

bool saveDevicemap() {
	if (persistence) return variantMapToJSONFile(inventory, DEVICESMAPFILE);
	return true;
}

void loadDevicemap() {
	inventory = jsonFileToVariantMap(DEVICESMAPFILE);
}

void get_sysinfo() {
#ifndef __FreeBSD__
	/* Note on FreeBSD exclusion. Sysinfo.h does not exist, but the code below
	 * does not really use it anyway.. so just skip it.
	 */ 
	struct sysinfo s_info;
	int error;
	error = sysinfo(&s_info);
	if(error != 0) {
		printf("code error = %d\n", error);
	} else { /*
		systeminfo["uptime"] = s_info.uptime;
		systeminfo["loadavg1"] = s_info.loads[0];
		systeminfo["loadavg5"] = s_info.loads[1];
		systeminfo["loadavg15"] = s_info.loads[2];
		systeminfo["totalram"] = s_info.totalram;
		systeminfo["freeram"] = s_info.freeram;
		systeminfo["procs"] = s_info.procs;
		*/
	}
#endif
}

bool emitNameEvent(const char *uuid, const char *eventType, const char *name) {
        Variant::Map content;
        content["name"] = name;
        content["uuid"] = uuid;
        return agoConnection->sendMessage(eventType, content);
}

bool emitFloorplanEvent(const char *uuid, const char *eventType, const char *floorplan, int x, int y) {
        Variant::Map content;
        content["uuid"] = uuid;
        content["floorplan"] = floorplan;
        content["x"] = x;
        content["y"] = y;
        return agoConnection->sendMessage(eventType, content);
}

// helper to determine last element
template <typename Iter>
Iter next(Iter iter)
{
	return ++iter;
}

string valuesToString(Variant::Map *values) {
	string result;
	for (Variant::Map::const_iterator it = values->begin(); it != values->end(); ++it) {
		result += it->second.asString();
		if ((it != values->end()) && (next(it) != values->end())) result += "/";	
	}
	return result;
}

// handles events that update the state or values of a device
void handleEvent(Variant::Map *device, string subject, Variant::Map *content) {
	Variant::Map *values;
	if ((*device)["values"].isVoid())  {
		cout << "error, device[values] is empty in handleEvent()" << endl;
		return;
	}
	values = &(*device)["values"].asMap();
	if ((subject == "event.device.statechanged") || (subject == "event.security.sensortriggered")) {
		(*values)["state"] = (*content)["level"];
		(*device)["state"]  = (*content)["level"];
		(*device)["state"].setEncoding("utf8");
		saveDevicemap();
		// (*device)["state"]  = valuesToString(values);
	} else if (subject == "event.environment.positionchanged") {
		Variant::Map value;
		stringstream timestamp;

		value["unit"] = (*content)["unit"];
		value["latitude"] = (*content)["latitude"];
		value["longitude"] = (*content)["longitude"];

		timestamp << time(NULL);
		value["timestamp"] = timestamp.str();

		(*values)["position"] = value;
		saveDevicemap();

	} else if ((subject.find("event.environment.") != std::string::npos) && (subject.find("changed")!= std::string::npos)) {
		Variant::Map value;
		stringstream timestamp;
		string quantity = subject;

		quantity.erase(quantity.begin(),quantity.begin()+18);
		quantity.erase(quantity.end()-7,quantity.end());

		value["unit"] = (*content)["unit"];
		value["level"] = (*content)["level"];

		timestamp << time(NULL);
		value["timestamp"] = timestamp.str();

		(*values)[quantity] = value;
		saveDevicemap();
	}
}

qpid::types::Variant::Map commandHandler(qpid::types::Variant::Map content) {
	std::string internalid = content["internalid"].asString();
	qpid::types::Variant::Map reply;
	if (internalid == "agocontroller") {
		if (content["command"] == "setroomname") {
			string uuid = content["room"];
			// if no uuid is provided, we need to generate one for a new room
			if (uuid == "") uuid = generateUuid();
			if (inv->setroomname(uuid, content["name"]) == 0) {
				reply["uuid"] = uuid;
				reply["returncode"] = 0;
				emitNameEvent(uuid.c_str(), "event.system.roomnamechanged", content["name"].asString().c_str());
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "setdeviceroom") {
			if ((content["device"].asString() != "") && (inv->setdeviceroom(content["device"], content["room"]) == 0)) {
				reply["returncode"] = 0;
				// update room in local device map
				Variant::Map *device;
				string room = inv->getdeviceroom(content["device"]);
				string uuid = content["device"];
				if (!inventory[uuid].isVoid()) {
					device = &inventory[uuid].asMap();
					(*device)["room"]= room;
				}
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "setdevicename") {
			if ((content["device"].asString() != "") && (inv->setdevicename(content["device"], content["name"]) == 0)) {
				reply["returncode"] = 0;
				// update name in local device map
				Variant::Map *device;
				string name = inv->getdevicename(content["device"]);
				string uuid = content["device"];
				if (!inventory[uuid].isVoid()) {
					device = &inventory[uuid].asMap();
					(*device)["name"]= name;
				}
				saveDevicemap();
				emitNameEvent(content["device"].asString().c_str(), "event.system.devicenamechanged", content["name"].asString().c_str());
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "deleteroom") {
			if (inv->deleteroom(content["room"]) == 0) {
				string uuid = content["room"].asString();
				emitNameEvent(uuid.c_str(), "event.system.roomdeleted", "");
				reply["returncode"] = 0;
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "setfloorplanname") {
			string uuid = content["floorplan"];
			// if no uuid is provided, we need to generate one for a new floorplan
			if (uuid == "") uuid = generateUuid();
			if (inv->setfloorplanname(uuid, content["name"]) == 0) {
				reply["uuid"] = uuid;
				reply["returncode"] = 0;
				emitNameEvent(content["floorplan"].asString().c_str(), "event.system.floorplannamechanged", content["name"].asString().c_str());
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "setdevicefloorplan") {
			if ((content["device"].asString() != "") && (inv->setdevicefloorplan(content["device"], content["floorplan"], content["x"], content["y"]) == 0)) {
				reply["returncode"] = 0;
				emitFloorplanEvent(content["device"].asString().c_str(), "event.system.floorplandevicechanged", content["floorplan"].asString().c_str(), content["x"], content["y"]);
			} else {
				reply["returncode"] = -1;
			}

		} else if (content["command"] == "deletefloorplan") {
			if (inv->deletefloorplan(content["floorplan"]) == 0) {
				reply["returncode"] = 0;
				emitNameEvent(content["floorplan"].asString().c_str(), "event.system.floorplandeleted", "");
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "setvariable") {
			if (content["variable"].asString() != "" && content["value"].asString() != "") {
				variables[content["variable"].asString()] = content["value"].asString();
				if (variantMapToJSONFile(variables, VARIABLESMAPFILE)) {
					reply["returncode"] = 0;
				} else {
					reply["returncode"] = -1;
				}
			} else {
				reply["returncode"] = -1;
			}
		} else if (content["command"] == "delvariable") {
			if (content["variable"].asString() != "") {
				Variant::Map::iterator it = variables.find(content["variable"].asString());
				if (it != variables.end()) {
					variables.erase(it);
					if (variantMapToJSONFile(variables, VARIABLESMAPFILE)) {
						reply["returncode"] = 0;
					} else {
						reply["returncode"] = -1;
					}
				} else {
					reply["returncode"] = -1;
				}
			}
		} else if (content["command"] == "getdevice") {
			if (content["device"].asString() != "") {
				if (!(inventory[content["device"].asString()].isVoid())) {
					reply["device"] = inventory[content["device"].asString()].asMap();
					reply["returncode"] = 0;
				} else {
					reply["returncode"] = -1;
				}
			} else {
				reply["returncode"] = -1;
			}
		}
	} else {
		if (content["command"] == "inventory") {
			// cout << "responding to inventory request" << std::endl;
			for (qpid::types::Variant::Map::iterator it = inventory.begin(); it != inventory.end(); it++) {
				if (!it->second.isVoid()) {
					qpid::types::Variant::Map *device = &it->second.asMap();
					if (time(NULL) - (*device)["lastseen"].asUint64() > 2*discoverdelay) {
						// cout << "Stale device: " << it->first << endl;
						(*device)["stale"] = 1;
						saveDevicemap();
					}
				}
			}
			reply["devices"] = inventory;
			reply["schema"] = schema;	
			reply["rooms"] = inv->getrooms();
			reply["floorplans"] = inv->getfloorplans();
			get_sysinfo();
			reply["system"] = systeminfo;
			reply["variables"] = variables;
			reply["environment"] = environment;
			reply["returncode"] = 0;
		}
	}
	return reply;
}

void eventHandler(std::string subject, qpid::types::Variant::Map content) {
	if (subject == "event.device.announce") {
		string uuid = content["uuid"];
		if (uuid != "") {
			// clog << agocontrol::kLogDebug << "preparing device: uuid="  << uuid << std::endl;
			Variant::Map device;
			Variant::Map values;
			device["devicetype"]=content["devicetype"].asString();
			device["internalid"]=content["internalid"].asString();
			device["handled-by"]=content["handled-by"].asString();
			// clog << agocontrol::kLogDebug << "getting name from inventory" << endl;
			device["name"]=inv->getdevicename(content["uuid"].asString());
			if (device["name"].asString() == "" && device["devicetype"] == "agocontroller") device["name"]="agocontroller";
			device["name"].setEncoding("utf8");
			// clog << agocontrol::kLogDebug << "getting room from inventory" << endl;
			device["room"]=inv->getdeviceroom(content["uuid"].asString()); 
			device["room"].setEncoding("utf8");
			uint64_t timestamp;
			timestamp = time(NULL);
			device["lastseen"] = timestamp;
			device["stale"] = 0;
			qpid::types::Variant::Map::const_iterator it = inventory.find(uuid);
			if (it == inventory.end()) {
				// device is newly announced, set default state and values
				device["state"]="0";
				device["state"].setEncoding("utf8");
				device["values"]=values;
				cout << "adding device: uuid="  << uuid  << " type: " << device["devicetype"].asString() << std::endl;
			} else {
				// device exists, get current values
				// TODO: use a non-const interator and modify the timestamp in place to avoid the following copying of data
				qpid::types::Variant::Map olddevice;
				if (!it->second.isVoid())  {
					olddevice= it->second.asMap();
					device["state"] = olddevice["state"];
					device["values"] = olddevice["values"];
				}
			}
				
			// clog << agocontrol::kLogDebug << "adding device: uuid="  << uuid  << " type: " << device["devicetype"].asString() << std::endl;
			inventory[uuid] = device;
			saveDevicemap();
		}
	} else if (subject == "event.device.remove") {
		string uuid = content["uuid"];
		if (uuid != "") {
			// clog << agocontrol::kLogDebug << "removing device: uuid="  << uuid  << std::endl;
			Variant::Map::iterator it = inventory.find(uuid);
			if (it != inventory.end()) {
				inventory.erase(it);
				saveDevicemap();
			}
		}
	} else if (subject == "event.environment.timechanged") {
		variables["hour"] = content["hour"].asString();
		variables["day"] = content["day"].asString();
		variables["weekday"] = content["weekday"].asString();
		variables["minute"] = content["minute"].asString();
		variables["month"] = content["month"].asString();
	} else {
		if (subject == "event.environment.positionchanged") {
			environment["latitude"] = content["latitude"];
			environment["longitude"] = content["longitude"];
		}
		if (content["uuid"].asString() != "") {
			string uuid = content["uuid"];
			// see if we have that device in the inventory already, if yes handle the event
			if (inventory.find(uuid) != inventory.end()) {
				if (!inventory[uuid].isVoid()) handleEvent(&inventory[uuid].asMap(), subject, &content);
			}
		}

	}
}

void *discover(void *param) {
	Variant::Map discovercmd;
	discovercmd["command"] = "discover";
	sleep(2);
	while (true) {
		agoConnection->sendMessage("",discovercmd);
		sleep(discoverdelay);
	}
	return NULL;
}

int main(int argc, char **argv) {
//	clog.rdbuf(new agocontrol::Log("agoresolver", LOG_LOCAL0));

	agoConnection = new AgoConnection("resolver");
	agoConnection->addHandler(commandHandler);
	agoConnection->addEventHandler(eventHandler);
	agoConnection->setFilter(false);

	string schemaPrefix;

//	clog << agocontrol::kLogNotice << "starting up" << std::endl;

	schemaPrefix=getConfigOption("system", "schemapath", CONFDIR "/schema.d/");
	discoverdelay=atoi(getConfigOption("system", "discoverdelay", "300").c_str());
	if (atoi(getConfigOption("system","devicepersistence", "0").c_str()) != 1) persistence=false;

	systeminfo["uuid"] = getConfigOption("system", "uuid", "00000000-0000-0000-000000000000");
	systeminfo["version"] = AGOCONTROL_VERSION;

	// generate vector of all schema files
	std::vector<std::string> schemaArray;
	fs::path schemadir(schemaPrefix.c_str());
	if (fs::exists(schemadir)) {
		fs::recursive_directory_iterator it(schemadir);
		fs::recursive_directory_iterator endit;
		while (it != endit) {
			if (fs::is_regular_file(*it) && (it->path().extension().string() == ".yaml")) {
				schemaArray.push_back(it->path().filename().string());
			}
			++it;
		}
	}
	if (schemaArray.size() < 1) {
		clog << agocontrol::kLogEmerg << "Can't read schema, aborting!" << std::endl;
		exit(1);
	}

	// load schema files in proper order
	std::sort(schemaArray.begin(), schemaArray.end());
	std::string schemaFile = schemaPrefix + schemaArray.front();
	schema = parseSchema(schemaFile.c_str());
	clog << agocontrol::kLogDebug << "parsing schema file:" << schemaFile << std::endl;
	for (int i=1; i < schemaArray.size(); i++) {
		schemaFile = schemaPrefix + schemaArray[i];
		clog << agocontrol::kLogDebug << "parsing additional schema file:" << schemaFile << std::endl;
		schema = mergeMap(schema, parseSchema(schemaFile.c_str()));
	}

	clog << agocontrol::kLogDebug << "reading inventory" << std::endl;
	inv = new Inventory(CONFDIR "/db/inventory.db");

	variables = jsonFileToVariantMap(VARIABLESMAPFILE);
	if (persistence) loadDevicemap();

	agoConnection->addDevice("agocontroller","agocontroller");

	static pthread_t discoverThread;
	pthread_create(&discoverThread,NULL,discover,NULL);
	
	// discover devices
	clog << agocontrol::kLogDebug << "discovering devices" << std::endl;
	agoConnection->run();	
}

