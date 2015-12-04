/*
     Copyright (C) 2013 Harald Klein <hari@vt100.at>

     This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License.
     This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

     See the GNU General Public License for more details.

*/

#include <iostream>
#include <sstream>
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <limits.h>
#include <float.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "agoclient.h"

#include <openzwave/Options.h>
#include <openzwave/Manager.h>
#include <openzwave/Driver.h>
#include <openzwave/Node.h>
#include <openzwave/Group.h>
#include <openzwave/Notification.h>
#include <openzwave/platform/Log.h>
#include <openzwave/value_classes/ValueStore.h>
#include <openzwave/value_classes/Value.h>
#include <openzwave/value_classes/ValueBool.h>

#include "ZWApi.h"
#include "ZWaveNode.h"

using namespace std;
using namespace agocontrol;
using namespace OpenZWave;

bool debug = true;
bool polling = false;

int unitsystem = 0; // 0 == SI, 1 == US

AgoConnection *agoConnection;

static uint32 g_homeId = 0;
static bool   g_initFailed = false;

typedef struct
{
	uint32			m_homeId;
	uint8			m_nodeId;
	bool			m_polled;
	list<ValueID>	m_values;
}NodeInfo;

static list<NodeInfo*> g_nodes;

static map<ValueID, qpid::types::Variant> valueCache;

static pthread_mutex_t g_criticalSection;
static pthread_cond_t  initCond  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

ZWaveNodes devices;

/*
class MyLog : public i_LogImpl {
	virtual void Write( LogLevel _level, uint8 const _nodeId, char const* _format, va_list _args );
};

void MyLog::Write( LogLevel _level, uint8 const _nodeId, char const* _format, va_list _args ) {
	char lineBuf[1024] = {};
	int lineLen = 0;
	if( _format != NULL && _format[0] != '\0' )
	{
		va_list saveargs;
		va_copy( saveargs, _args );
		lineLen = vsnprintf( lineBuf, sizeof(lineBuf), _format, _args );
		va_end( saveargs );
	}
	cout << string(lineBuf) << endl;
}
*/
const char *controllerErrorStr (Driver::ControllerError err)
{
	switch (err) {
		case Driver::ControllerError_None:
			return "None";
		case Driver::ControllerError_ButtonNotFound:
			return "Button Not Found";
		case Driver::ControllerError_NodeNotFound:
			return "Node Not Found";
		case Driver::ControllerError_NotBridge:
			return "Not a Bridge";
		case Driver::ControllerError_NotPrimary:
			return "Not Primary Controller";
		case Driver::ControllerError_IsPrimary:
			return "Is Primary Controller";
		case Driver::ControllerError_NotSUC:
			return "Not Static Update Controller";
		case Driver::ControllerError_NotSecondary:
			return "Not Secondary Controller";
		case Driver::ControllerError_NotFound:
			return "Not Found";
		case Driver::ControllerError_Busy:
			return "Controller Busy";
		case Driver::ControllerError_Failed:
			return "Failed";
		case Driver::ControllerError_Disabled:
			return "Disabled";
		case Driver::ControllerError_Overflow:
			return "Overflow";
		default:
			return "Unknown error";
	}
}

void controller_update(Driver::ControllerState state,  Driver::ControllerError err, void *context) {
	printf("controller state update:");
	qpid::types::Variant::Map eventmap;
	eventmap["statecode"]=state;
	switch(state) {
		case Driver::ControllerState_Normal:
			printf("no command in progress");
			eventmap["state"]="normal";
			eventmap["description"]="Normal: No command in progress";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			// nothing to do
			break;
		case Driver::ControllerState_Waiting:
			printf("waiting for user action");
			eventmap["state"]="awaitaction";
			eventmap["description"]="Waiting for user action";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			// waiting for user action
			break;
		case Driver::ControllerState_Cancel:
			printf("command was cancelled");
			eventmap["state"]="cancel";
			eventmap["description"]="Command was cancelled";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
		case Driver::ControllerState_Error:
			printf("command returned error");
			eventmap["state"]="error";
			eventmap["description"]="Command returned error";
			eventmap["error"] = err;
			eventmap["errorstring"] = controllerErrorStr(err);
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
		case Driver::ControllerState_Sleeping:
			printf("device went to sleep");
			eventmap["state"]="sleep";
			eventmap["description"]="Device went to sleep";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;

		case Driver::ControllerState_InProgress:
			printf("communicating with other device");
			eventmap["state"]="inprogress";
			eventmap["description"]="Communication in progress";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			// communicating with device
			break;
		case Driver::ControllerState_Completed:
			printf("command has completed successfully");
			eventmap["state"]="success";
			eventmap["description"]="Command completed";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
		case Driver::ControllerState_Failed:
			printf("command has failed");
			eventmap["state"]="failed";
			eventmap["description"]="Command failed";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			// houston..
			break;
		case Driver::ControllerState_NodeOK:
			printf("node ok");
			eventmap["state"]="nodeok";
			eventmap["description"]="Node OK";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
		case Driver::ControllerState_NodeFailed:
			printf("node failed");
			eventmap["state"]="nodefailed";
			eventmap["description"]="Node failed";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
		default:
			printf("unknown response");
			eventmap["state"]="unknown";
			eventmap["description"]="Unknown response";
			agoConnection->emitEvent("zwavecontroller", "event.zwave.controllerstate", eventmap);
			break;
	}
	printf("\n");
	if (err != Driver::ControllerError_None)  {
		printf("%s\n", controllerErrorStr(err));
	}
}

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
NodeInfo* GetNodeInfo
(
	Notification const* _notification
)
{
	uint32 const homeId = _notification->GetHomeId();
	uint8 const nodeId = _notification->GetNodeId();
	for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
	{
		NodeInfo* nodeInfo = *it;
		if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
		{
			return nodeInfo;
		}
	}

	return NULL;
}


ValueID* getValueID(int nodeid, int instance, string label) {
        for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
        {
		for (list<ValueID>::iterator it2 = (*it)->m_values.begin(); it2 != (*it)->m_values.end(); it2++ ) {
			// printf("Node ID: %3d Value ID: %d\n", (*it)->m_nodeId, (*it2).GetId());
			if ( ((*it)->m_nodeId == nodeid) && ((*it2).GetInstance() == instance) ) {
				string valuelabel = Manager::Get()->GetValueLabel((*it2));
				if (label == valuelabel) {
					// printf("Found ValueID: %d\n",(*it2).GetId());
					return &(*it2);
				}
			}
		}
	}
	return NULL;
}

string uint64ToString(uint64_t i) {
	stringstream tmp;
	tmp << i;
	return tmp.str();
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OnNotification
(
	Notification const* _notification,
	void* _context
)
{
	qpid::types::Variant::Map eventmap;
	// Must do this inside a critical section to avoid conflicts with the main thread
	pthread_mutex_lock( &g_criticalSection );

	switch( _notification->GetType() )
	{
		case Notification::Type_ValueAdded:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Add the new value to our list
				nodeInfo->m_values.push_back( _notification->GetValueID() );
				uint8 basic = Manager::Get()->GetNodeBasic(_notification->GetHomeId(),_notification->GetNodeId());
				uint8 generic = Manager::Get()->GetNodeGeneric(_notification->GetHomeId(),_notification->GetNodeId());
				uint8 specific = Manager::Get()->GetNodeSpecific(_notification->GetHomeId(),_notification->GetNodeId());
				ValueID id = _notification->GetValueID();
				string label = Manager::Get()->GetValueLabel(id);
				stringstream tempstream;
				tempstream << (int) _notification->GetNodeId();
				tempstream << "/";
				tempstream << (int) id.GetInstance();
				string nodeinstance = tempstream.str();
				tempstream << "-";
				tempstream << label;
				string tempstring = tempstream.str();
				ZWaveNode *device;
				if (id.GetGenre() == ValueID::ValueGenre_Config) {
					printf("CONFIGURATION PARAMETER Value Added Home 0x%08x Node %d Genre %d Class %x Instance %d Index %d Type %d - Label: %s\n", _notification->GetHomeId(), _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(), id.GetInstance(), id.GetIndex(), id.GetType(),label.c_str());


				} else if (basic == BASIC_TYPE_CONTROLLER) {
					if ((device = devices.findId(nodeinstance)) != NULL) {
						device->addValue(label, id);
						device->setDevicetype("remote");
					} else {
						device = new ZWaveNode(nodeinstance, "remote");	
						device->addValue(label, id);
						devices.add(device);
						agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
					}
				} else
				switch(id.GetCommandClassId()) {
					case COMMAND_CLASS_SWITCH_MULTILEVEL:
						if (label == "Level") {
							if ((device = devices.findId(nodeinstance)) != NULL) {
								device->addValue(label, id);
								device->setDevicetype("dimmer");
							} else {
								device = new ZWaveNode(nodeinstance, "dimmer");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
							// Manager::Get()->EnablePoll(id);
						}
					break;
					case COMMAND_CLASS_SWITCH_BINARY:
						if (label == "Switch") {
							if ((device = devices.findId(nodeinstance)) != NULL) {
								device->addValue(label, id);
							} else {
								device = new ZWaveNode(nodeinstance, "switch");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
							// Manager::Get()->EnablePoll(id);
						}
					break;
					case COMMAND_CLASS_SENSOR_BINARY:
						if (label == "Sensor") {
							if ((device = devices.findId(tempstring)) != NULL) {
								device->addValue(label, id);
							} else {
								device = new ZWaveNode(tempstring, "binarysensor");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
							// Manager::Get()->EnablePoll(id);
						}
					break;
					case COMMAND_CLASS_SENSOR_MULTILEVEL:
						if (label == "Luminance") {
							device = new ZWaveNode(tempstring, "brightnesssensor");	
							device->addValue(label, id);
							devices.add(device);
							agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
						} else if (label == "Temperature") {
							if (generic == GENERIC_TYPE_THERMOSTAT) {
								if ((device = devices.findId(nodeinstance)) != NULL) {
									device->addValue(label, id);
								} else {
									device = new ZWaveNode(nodeinstance, "thermostat");	
									device->addValue(label, id);
									devices.add(device);
									agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
								}
							} else {
								device = new ZWaveNode(tempstring, "temperaturesensor");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
						} else {
							printf("WARNING: unhandled label for SENSOR_MULTILEVEL: %s - adding generic multilevelsensor\n",label.c_str());
							if ((device = devices.findId(nodeinstance)) != NULL) {
								device->addValue(label, id);
							} else {
								device = new ZWaveNode(nodeinstance, "multilevelsensor");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
						}
						// Manager::Get()->EnablePoll(id);
					break;
					case COMMAND_CLASS_METER:
						if (label == "Power") {
							device = new ZWaveNode(tempstring, "powermeter");	
							device->addValue(label, id);
							devices.add(device);
							agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
						} else if (label == "Energy") {
							device = new ZWaveNode(tempstring, "energymeter");	
							device->addValue(label, id);
							devices.add(device);
							agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
						} else {
							printf("WARNING: unhandled label for CLASS_METER: %s - adding generic multilevelsensor\n",label.c_str());
							if ((device = devices.findId(nodeinstance)) != NULL) {
								device->addValue(label, id);
							} else {
								device = new ZWaveNode(nodeinstance, "multilevelsensor");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
						}
						// Manager::Get()->EnablePoll(id);
					break;
					case COMMAND_CLASS_BASIC_WINDOW_COVERING:
						// if (label == "Open") {
							if ((device = devices.findId(nodeinstance)) != NULL) {
								device->addValue(label, id);
								device->setDevicetype("drapes");
							} else {
								device = new ZWaveNode(nodeinstance, "drapes");	
								device->addValue(label, id);
								devices.add(device);
								agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
							}
							// Manager::Get()->EnablePoll(id);
					//	}
					break;
					case COMMAND_CLASS_THERMOSTAT_SETPOINT:
						if (polling) Manager::Get()->EnablePoll(id,1);
					case COMMAND_CLASS_THERMOSTAT_MODE:
					case COMMAND_CLASS_THERMOSTAT_FAN_MODE:
					case COMMAND_CLASS_THERMOSTAT_FAN_STATE:
					case COMMAND_CLASS_THERMOSTAT_OPERATING_STATE:
						cout << "adding thermostat label: " << label << endl;
						if ((device = devices.findId(nodeinstance)) != NULL) {
							device->addValue(label, id);
							device->setDevicetype("thermostat");
						} else {
							device = new ZWaveNode(nodeinstance, "thermostat");	
							device->addValue(label, id);
							devices.add(device);
							agoConnection->addDevice(device->getId().c_str(), device->getDevicetype().c_str());
						}
					break;
					default:
						printf("Notification: Unassigned Value Added Home 0x%08x Node %d Genre %d Class %x Instance %d Index %d Type %d - Label: %s\n", _notification->GetHomeId(), _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(), id.GetInstance(), id.GetIndex(), id.GetType(),label.c_str());
						// printf("Notification: Unassigned Value Added Home 0x%08x Node %d Genre %d Class %x Instance %d Index %d Type %d - ID: %" PRIu64 "\n", _notification->GetHomeId(), _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(), id.GetInstance(), id.GetIndex(), id.GetType(),id.GetId());

				}
			}
			break;
		}
		case Notification::Type_ValueRemoved:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// Remove the value from out list
				for( list<ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it )
				{
					if( (*it) == _notification->GetValueID() )
					{
						nodeInfo->m_values.erase( it );
						break;
					}
				}
			}
			break;
		}

		case Notification::Type_ValueChanged:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// One of the node values has changed
				// TBD...
				// nodeInfo = nodeInfo;
				ValueID id = _notification->GetValueID();
				string str;
				printf("Notification: Value Changed Home 0x%08x Node %d Genre %d Class %d Instance %d Index %d Type %d\n", _notification->GetHomeId(), _notification->GetNodeId(), id.GetGenre(), id.GetCommandClassId(), id.GetInstance(), id.GetIndex(), id.GetType());
			      if (Manager::Get()->GetValueAsString(id, &str)) {
					qpid::types::Variant cachedValue;
					cachedValue.parse(str);
					valueCache[id] = cachedValue;
					string label = Manager::Get()->GetValueLabel(id);
					string units = Manager::Get()->GetValueUnits(id);

					// TODO: send proper types and don't convert everything to string
					string level = str;
					string eventtype = "";
					if (str == "True") level="255";
					if (str == "False") level="0";
					printf("Value: %s Label: %s Unit: %s\n",str.c_str(),label.c_str(),units.c_str());
					if ((label == "Basic") || (label == "Switch") || (label == "Level")) {
						eventtype="event.device.statechanged";
					}
					if (label == "Luminance") {
						eventtype="event.environment.brightnesschanged";
					}
					if (label == "Temperature") {
						eventtype="event.environment.temperaturechanged";
						if (units=="F" && unitsystem==0) {
							units="C";
							str = float2str((atof(str.c_str())-32)*5/9);
							level = str;
						} else if (units =="C" && unitsystem==1) {
							units="F";
							str = float2str(atof(str.c_str())*9/5 + 32);
							level = str;
						}
					}
					if (label == "Relative Humidity") {
						eventtype="event.environment.humiditychanged";
					}
					if (label == "Battery Level") {
						eventtype="event.device.batterylevelchanged";
					}
					if (label == "Alarm Level") {
						eventtype="event.security.alarmlevelchanged";
					}
					if (label == "Alarm Type") {
						eventtype="event.security.alarmtypechanged";
					}
					if (label == "Sensor") {
						eventtype="event.security.sensortriggered";
					}
					if (label == "Energy") {
						eventtype="event.environment.energychanged";
					}
					if (label == "Power") {
						eventtype="event.environment.powerchanged";
					}
					if (label == "Mode") {
						eventtype="event.environment.modechanged";
					}
					if (label == "Fan Mode") {
						eventtype="event.environment.fanmodechanged";
					}
					if (label == "Fan State") {
						eventtype="event.environment.fanstatechanged";
					}
					if (label == "Operating State") {
						eventtype="event.environment.operatingstatechanged";
					}
					if (label == "Cooling 1") {
						eventtype="event.environment.coolsetpointchanged";
					}
					if (label == "Heating 1") {
						eventtype="event.environment.heatsetpointchanged";
					}
					if (label == "Fan State") {
						eventtype="event.environment.fanstatechanged";
					}
					if (eventtype != "") {	
						ZWaveNode *device = devices.findValue(id);
						if (device != NULL) {
							if (debug) printf("Sending %s event from child %s\n",eventtype.c_str(), device->getId().c_str());
							agoConnection->emitEvent(device->getId().c_str(), eventtype.c_str(), level.c_str(), units.c_str());	
						}
					}
				}
			}
			break;
		}
		case Notification::Type_Group:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// One of the node's association groups has changed
				// TBD...
				nodeInfo = nodeInfo;
				eventmap["description"]="Node association added";
				eventmap["state"]="associationchanged";
				eventmap["nodeid"] = _notification->GetNodeId();
				eventmap["homeid"] = _notification->GetHomeId();
				agoConnection->emitEvent("zwavecontroller", "event.zwave.associationchanged", eventmap);
			}
			break;
		}

		case Notification::Type_NodeAdded:
		{
			// Add the new node to our list
			NodeInfo* nodeInfo = new NodeInfo();
			nodeInfo->m_homeId = _notification->GetHomeId();
			nodeInfo->m_nodeId = _notification->GetNodeId();
			nodeInfo->m_polled = false;		
			g_nodes.push_back( nodeInfo );

			// todo: announce node
			eventmap["description"]="Node added";
			eventmap["state"]="nodeadded";
			eventmap["nodeid"] = _notification->GetNodeId();
			eventmap["homeid"] = _notification->GetHomeId();
			agoConnection->emitEvent("zwavecontroller", "event.zwave.networkchanged", eventmap);
			break;
		}

		case Notification::Type_NodeRemoved:
		{
			// Remove the node from our list
			uint32 const homeId = _notification->GetHomeId();
			uint8 const nodeId = _notification->GetNodeId();
			eventmap["description"]="Node removed";
			eventmap["state"]="noderemoved";
			eventmap["nodeid"] = _notification->GetNodeId();
			eventmap["homeid"] = _notification->GetHomeId();
			agoConnection->emitEvent("zwavecontroller", "event.zwave.networkchanged", eventmap);
			for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
			{
				NodeInfo* nodeInfo = *it;
				if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
				{
					g_nodes.erase( it );
					break;
				}
			}
			break;
		}

		case Notification::Type_NodeEvent:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				// We have received an event from the node, caused by a
				// basic_set or hail message.
				ValueID id = _notification->GetValueID();
				string label = Manager::Get()->GetValueLabel(id);
				stringstream level;
				level << (int) _notification->GetByte();
				string eventtype = "event.device.statechanged";
				ZWaveNode *device = devices.findValue(id);
				if (device != NULL) {
					if (debug) printf("Sending %s event from child %s\n",eventtype.c_str(), device->getId().c_str());
					agoConnection->emitEvent(device->getId().c_str(), eventtype.c_str(), level.str().c_str(), "");	
				} else {
					cout << "no agocontrol device found for node event - Label: " << label << " Level: " << level << endl;
				}

			}
			break;
		}
		case Notification::Type_SceneEvent:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				int scene = _notification->GetSceneId();
				ValueID id = _notification->GetValueID();
                                string label = Manager::Get()->GetValueLabel(id);
                                stringstream tempstream;
                                tempstream << (int) _notification->GetNodeId();
                                tempstream << "/1";
                                string nodeinstance = tempstream.str();
				string eventtype = "event.device.scenechanged";
				ZWaveNode *device;
				if ((device = devices.findId(nodeinstance)) != NULL) {
					if (debug) printf("Sending %s for scene %d event from child %s\n",
						  eventtype.c_str(), scene, device->getId().c_str());
					qpid::types::Variant::Map content;
					content["scene"]=scene;
					agoConnection->emitEvent(device->getId().c_str(), eventtype.c_str(), content);	
				} else {
					cout << "WARNING: no agocontrol device found for scene event" << endl;
					cout << "Node: " << nodeinstance << " Scene: " << scene << endl;
				}

			}
			break;
		}
		case Notification::Type_PollingDisabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = false;
			}
			break;
		}

		case Notification::Type_PollingEnabled:
		{
			if( NodeInfo* nodeInfo = GetNodeInfo( _notification ) )
			{
				nodeInfo->m_polled = true;
			}
			break;
		}

		case Notification::Type_DriverReady:
		{
			g_homeId = _notification->GetHomeId();
			break;
		}


		case Notification::Type_DriverFailed:
		{
			g_initFailed = true;
			pthread_cond_broadcast(&initCond);
			break;
		}

		case Notification::Type_AwakeNodesQueried:
		case Notification::Type_AllNodesQueried:
		case Notification::Type_AllNodesQueriedSomeDead:
		{
			pthread_cond_broadcast(&initCond);
			break;
		}
		case Notification::Type_DriverReset:
		case Notification::Type_Notification:
		case Notification::Type_NodeNaming:
		case Notification::Type_NodeProtocolInfo:
		case Notification::Type_NodeQueriesComplete:
		case Notification::Type_EssentialNodeQueriesComplete:
		{
			break;
		}
		default:
		{
			cout << "WARNING: Unhandled OpenZWave Event: " << _notification->GetType() << endl;
		}
	}

	pthread_mutex_unlock( &g_criticalSection );
}



qpid::types::Variant::Map commandHandler(qpid::types::Variant::Map content) {
        qpid::types::Variant::Map returnval;
	bool result;
	std::string internalid = content["internalid"].asString();
	// printf("command: %s internal id: %s\n", content["command"].asString().c_str(), internalid.c_str());

	if (internalid == "zwavecontroller") {
		printf("z-wave specific controller command received\n");
		if (content["command"] == "addnode") {
			Manager::Get()->BeginControllerCommand(g_homeId, Driver::ControllerCommand_AddDevice, controller_update, NULL, true);
			result = true;
		} else if (content["command"] == "removenode") {
			if (!(content["node"].isVoid())) {
				int mynode = content["node"];
				Manager::Get()->BeginControllerCommand(g_homeId, Driver::ControllerCommand_RemoveFailedNode, controller_update, NULL, true, mynode);
			} else {
				Manager::Get()->BeginControllerCommand(g_homeId, Driver::ControllerCommand_RemoveDevice, controller_update, NULL, true);
			}
			result = true;
		} else if (content["command"] == "healnode") {
			if (!(content["node"].isVoid())) {
				int mynode = content["node"];
				Manager::Get()->HealNetworkNode(g_homeId, mynode, true);
			    result = true;
			}
            else {
                result = false;
            }
		} else if (content["command"] == "healnetwork") {
			Manager::Get()->HealNetwork(g_homeId, true);
			result = true;
		} else if (content["command"] == "refreshnode") {
			if (!(content["node"].isVoid())) {
				int mynode = content["node"];
				Manager::Get()->RefreshNodeInfo(g_homeId, mynode);
				result = true;
			}
            else {
                result = false;
            }
		} else if (content["command"] == "getstatistics") {
			Driver::DriverData data;
			Manager::Get()->GetDriverStatistics( g_homeId, &data );
			qpid::types::Variant::Map statistics;
			statistics["SOF"] = data.m_SOFCnt;
			statistics["ACK waiting"] = data.m_ACKWaiting;
			statistics["Read Aborts"] = data.m_readAborts;
			statistics["Bad Checksums"] = data.m_badChecksum;
			statistics["Reads"] = data.m_readCnt;
			statistics["Writes"] = data.m_writeCnt;
			statistics["CAN"] = data.m_CANCnt;
			statistics["NAK"] = data.m_NAKCnt;
			statistics["ACK"] = data.m_ACKCnt;
			statistics["OOF"] = data.m_OOFCnt;
			statistics["Dropped"] = data.m_dropped;
			statistics["Retries"] = data.m_retries;
			returnval["statistics"]=statistics;
			result = true;
		} else if (content["command"] == "testnode") {
			if (!(content["node"].isVoid())) {
				int mynode = content["node"];
				int count = 10;
				if (!(content["count"].isVoid())) count=content["count"];
                                Manager::Get()->TestNetworkNode(g_homeId, mynode, count);
                                result = true;
                        }
        } else if (content["command"] == "testnetwork") {
            int count = 10;
            if (!(content["count"].isVoid()))
                count=content["count"];
            Manager::Get()->TestNetwork(g_homeId, count);
            result = true;
        } else if (content["command"] == "getnodes") {
			qpid::types::Variant::Map nodelist;
			for( list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it )
			{
				NodeInfo* nodeInfo = *it;
				string index;
				qpid::types::Variant::Map node;
				qpid::types::Variant::List neighborsList;
				qpid::types::Variant::List valuesList;
				qpid::types::Variant::Map status;

				uint8* neighbors;
				uint32 numNeighbors = Manager::Get()->GetNodeNeighbors(nodeInfo->m_homeId,nodeInfo->m_nodeId,&neighbors);
				if (numNeighbors) {
					for(uint32 i=0; i<numNeighbors; i++) {
						neighborsList.push_back(neighbors[i]);
					}
					delete [] neighbors;
				}
				node["neighbors"]=neighborsList;	

				for (list<ValueID>::iterator it2 = (*it)->m_values.begin(); it2 != (*it)->m_values.end(); it2++ ) {
					ZWaveNode *device = devices.findValue(*it2);
					if (device != NULL) {
						valuesList.push_back(device->getId());
					}
				}
				node["internalids"] = valuesList;
			
				node["manufacturer"]=Manager::Get()->GetNodeManufacturerName(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["version"]=Manager::Get()->GetNodeVersion(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["basic"]=Manager::Get()->GetNodeBasic(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["generic"]=Manager::Get()->GetNodeGeneric(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["specific"]=Manager::Get()->GetNodeSpecific(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["product"]=Manager::Get()->GetNodeProductName(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["type"]=Manager::Get()->GetNodeType(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["producttype"]=Manager::Get()->GetNodeProductType(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				node["numgroups"]=Manager::Get()->GetNumGroups(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				status["querystage"]=Manager::Get()->GetNodeQueryStage(nodeInfo->m_homeId,nodeInfo->m_nodeId);
				status["awake"]=(Manager::Get()->IsNodeAwake(nodeInfo->m_homeId,nodeInfo->m_nodeId) ? 1 : 0);
				status["listening"]=(Manager::Get()->IsNodeListeningDevice(nodeInfo->m_homeId,nodeInfo->m_nodeId) || 
					Manager::Get()->IsNodeFrequentListeningDevice(nodeInfo->m_homeId,nodeInfo->m_nodeId) ? 1 : 0);
				status["failed"]=(Manager::Get()->IsNodeFailed(nodeInfo->m_homeId,nodeInfo->m_nodeId) ? 1 : 0);
				node["status"]=status;

				uint8 nodeid = nodeInfo->m_nodeId;
				index = static_cast<ostringstream*>( &(ostringstream() << nodeid) )->str();
				nodelist[index.c_str()] = node;
			//	if( ( nodeInfo->m_homeId == homeId ) && ( nodeInfo->m_nodeId == nodeId ) )
			}
			returnval["nodelist"]=nodelist;
			result = true;
		} else if (content["command"] == "addassociation") {
			int mynode = content["node"];
			int mygroup = content["group"];
			int mytarget = content["target"];
			printf("adding association: %i %i %i\n",mynode,mygroup,mytarget);
			Manager::Get()->AddAssociation(g_homeId, mynode, mygroup, mytarget);
			result = true;
		} else if (content["command"] == "getassociations") {
			qpid::types::Variant::Map associationsmap;
			int mygroup = content["group"];
			int mynode = content["node"];
			uint8_t *associations;
			uint32_t numassoc = Manager::Get()->GetAssociations(g_homeId, mynode, mygroup, &associations);
			for (int assoc = 0; assoc < numassoc; assoc++) {
				associationsmap[int2str(assoc)] = associations[assoc];
			}
			if (numassoc >0) delete associations;
			returnval["associations"] = associationsmap;
			returnval["label"] = Manager::Get()->GetGroupLabel(g_homeId, mynode, mygroup);
			result = true;
		} else if (content["command"] == "removeassociation") {
			Manager::Get()->RemoveAssociation(g_homeId, content["node"], content["group"], content["target"]);
			result = true;
		} else if (content["command"] == "setconfigparam") {
			int mynode = content["node"];
			int myparam = content["param"];
			int myvalue = content["value"];
			int mysize = content["size"];
			printf("setting config param: node: %i param: %i size: %i value: %i\n",mynode,myparam,mysize,myvalue);
			result = Manager::Get()->SetConfigParam(g_homeId,mynode,myparam,myvalue,mysize); 
		} else if (content["command"] == "downloadconfig") {
			result = true;
			result = Manager::Get()->BeginControllerCommand(g_homeId, Driver::ControllerCommand_ReceiveConfiguration, controller_update, NULL, true);
		} else if (content["command"] == "cancel") {
			result = true;
			Manager::Get()->CancelControllerCommand(g_homeId);
		} else if (content["command"] == "saveconfig") {
			result = true;
			Manager::Get()->WriteConfig( g_homeId );
		} else if (content["command"] == "allon") {
			result = true;
			Manager::Get()->SwitchAllOn(g_homeId );
		} else if (content["command"] == "alloff") {
			result = true;
			Manager::Get()->SwitchAllOff(g_homeId );
		} else if (content["command"] == "reset") {
			result = true;
			Manager::Get()->ResetController(g_homeId);
		} else if( content["command"] == "setport" ) {
            if( !content["port"].isVoid() ) {
                int port = content["port"];
                result = setConfigOption("zwave", "device", port);
            }
            else {
                result = false;
            }
        }
        else if( content["command"] == "getport" ) {
            result = true;
            //return empty string if not configured
            returnval["port"] = getConfigOption("zwave", "device", "");
        }

	} else {
		ZWaveNode *device = devices.findId(internalid);
		if (device != NULL) {
			printf("command received for %s\n", internalid.c_str());
			printf("device type: %s\n", device->getDevicetype().c_str()); 

			string devicetype = device->getDevicetype();
			ValueID *tmpValueID;

			if (devicetype == "switch") {
				tmpValueID = device->getValueID("Switch");
				if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
				if (content["command"] == "on" ) {
					result = Manager::Get()->SetValue(*tmpValueID , true);
				} else {
					result = Manager::Get()->SetValue(*tmpValueID , false);
				}
			} else if(devicetype == "dimmer") {
				tmpValueID = device->getValueID("Level");
				if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
				if (content["command"] == "on" ) {
					result = Manager::Get()->SetValue(*tmpValueID , (uint8) 255);
				} else if (content["command"] == "setlevel") {
					uint8 level = atoi(content["level"].asString().c_str());
					if (level > 99) level=99;
					result = Manager::Get()->SetValue(*tmpValueID, level);
				} else {
					result = Manager::Get()->SetValue(*tmpValueID , (uint8) 0);
				}
			} else if (devicetype == "drapes") {
				if (content["command"] == "on") {
					tmpValueID = device->getValueID("Level");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					result = Manager::Get()->SetValue(*tmpValueID , (uint8) 255);
				} else if (content["command"] == "open" ) {
					tmpValueID = device->getValueID("Open");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					result = Manager::Get()->SetValue(*tmpValueID , true);
				} else if (content["command"] == "close" ) {
					tmpValueID = device->getValueID("Close");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					result = Manager::Get()->SetValue(*tmpValueID , true);
				} else if (content["command"] == "stop" ) {
					tmpValueID = device->getValueID("Stop");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					result = Manager::Get()->SetValue(*tmpValueID , true);

				} else {
					tmpValueID = device->getValueID("Level");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					result = Manager::Get()->SetValue(*tmpValueID , (uint8) 0);
				}
			} else if (devicetype == "thermostat") {
				if (content["command"] == "settemperature") {
					string mode = content["mode"].asString();
					if  (mode == "") mode = "auto";
					if (mode == "cool") {
						tmpValueID = device->getValueID("Cooling 1");
					} else if ((mode == "auto") || (mode == "heat")) {
						tmpValueID = device->getValueID("Heating 1");
					}
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					float temp = 0.0;
					if (content["temperature"] == "-1") {
						try {
							cout << "rel temp -1:" << valueCache[*tmpValueID] << endl;
							temp = atof(valueCache[*tmpValueID].asString().c_str());
							temp = temp - 1.0;
						} catch (...) {
							cout << "can't determine current value for relative temperature change" << endl;
							returnval["result"] = -1; return returnval;
						}
					} else if (content["temperature"] == "+1") {
						try {
							cout << "rel temp +1: " << valueCache[*tmpValueID] << endl;
							temp = atof(valueCache[*tmpValueID].asString().c_str());
							temp = temp + 1.0;
						} catch (...) {
							cout << "can't determine current value for relative temperature change" << endl;
							returnval["result"] = -1; return returnval;
						}
					} else {
						temp = content["temperature"];
					}
					cout << "setting temperature: " << temp << endl;
					result = Manager::Get()->SetValue(*tmpValueID , temp);
				} else if (content["command"] == "setthermostatmode") {
					string mode = content["mode"].asString();
					tmpValueID = device->getValueID("Mode");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					if (mode=="heat") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Heat");
					} else if (mode=="cool") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Cool");
					} else if (mode == "off")  {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Off");
					} else if (mode == "auxheat")  {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Aux Heat");
					} else {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Auto");
					}
				} else if (content["command"] == "setthermostatfanmode") {
					string mode = content["mode"].asString();
					tmpValueID = device->getValueID("Fan Mode");
					if (tmpValueID == NULL) { returnval["result"] = -1;  return returnval; }
					if (mode=="circulate") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Circulate");
					} else if (mode=="on" || mode=="onlow") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "On Low");
					} else if (mode=="onhigh") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "On High");
					} else if (mode=="autohigh") {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Auto High");
					} else {
						result = Manager::Get()->SetValueListSelection(*tmpValueID , "Auto Low");
					}

				}
			}

		}
			//printf("Type: %i - %s\n",tmpValueID->GetType(), Value::GetTypeNameFromEnum(tmpValueID->GetType()));

	}
	returnval["result"] = result ? 0 : -1;
	return returnval;
}

int main(int argc, char **argv) {
	std::string device;

	device=getConfigOption("zwave", "device", "/dev/usbzwave");
	if (getConfigOption("system", "units", "SI")!="SI") unitsystem=1;


	pthread_mutexattr_t mutexattr;

	pthread_mutexattr_init ( &mutexattr );
	pthread_mutexattr_settype( &mutexattr, PTHREAD_MUTEX_RECURSIVE );

	pthread_mutex_init( &g_criticalSection, &mutexattr );
	pthread_mutexattr_destroy( &mutexattr );

	pthread_mutex_lock( &initMutex );

	
	AgoConnection _agoConnection = AgoConnection("zwave");		
	agoConnection = &_agoConnection;
	printf("connection to agocontrol established\n");

	// init open zwave
	Options::Create( "/etc/openzwave/", CONFDIR "/ozw/", "" );
	Options::Get()->AddOptionBool("PerformReturnRoutes", false );
	Options::Get()->AddOptionBool("ConsoleOutput", false ); 
	Options::Get()->AddOptionBool("EnableSIS", true ); 

	Options::Get()->AddOptionInt( "SaveLogLevel", LogLevel_Detail );
	Options::Get()->AddOptionInt( "QueueLogLevel", LogLevel_Debug );
	Options::Get()->AddOptionInt( "DumpTrigger", LogLevel_Error );

	int retryTimeout = atoi(getConfigOption("zwave","retrytimeout","2000").c_str());
	OpenZWave::Options::Get()->AddOptionInt("RetryTimeout", retryTimeout);

	Options::Get()->Lock();

	// MyLog myLog;
	// Log::SetLoggingClass(myLog);
	OpenZWave::Log* pLog = OpenZWave::Log::Create("/var/log/zwave.log", true, false, OpenZWave::LogLevel_Info, OpenZWave::LogLevel_Debug, OpenZWave::LogLevel_Error);
        pLog->SetLogFileName("/var/log/zwave.log"); // Make sure, in case Log::Create already was called before we got here
        pLog->SetLoggingState(OpenZWave::LogLevel_Info, OpenZWave::LogLevel_Debug, OpenZWave::LogLevel_Error);



	Manager::Create();
	Manager::Get()->AddWatcher( OnNotification, NULL );
	// Manager::Get()->SetPollInterval(atoi(getConfigOption("zwave", "pollinterval", "300000").c_str()),true);
	if (getConfigOption("zwave", "polling", "0") == "1") polling=true;
	Manager::Get()->AddDriver(device);

	// Now we just wait for the driver to become ready
	printf("waiting for OZW driver to become ready\n");
	pthread_cond_wait( &initCond, &initMutex );
	printf("pthread_cond_wait returned\n");

	if( !g_initFailed )
	{

		Manager::Get()->WriteConfig( g_homeId );
		Driver::DriverData data;
		Manager::Get()->GetDriverStatistics( g_homeId, &data );
		printf("SOF: %d ACK Waiting: %d Read Aborts: %d Bad Checksums: %d\n", data.m_SOFCnt, data.m_ACKWaiting, data.m_readAborts, data.m_badChecksum);
		printf("Reads: %d Writes: %d CAN: %d NAK: %d ACK: %d Out of Frame: %d\n", data.m_readCnt, data.m_writeCnt, data.m_CANCnt, data.m_NAKCnt, data.m_ACKCnt, data.m_OOFCnt);
		printf("Dropped: %d Retries: %d\n", data.m_dropped, data.m_retries);

		printf("OZW startup complete\n");
		cout << devices.toString() << endl;

		agoConnection->addDevice("zwavecontroller", "zwavecontroller");
		agoConnection->addHandler(commandHandler);
		/* for (std::list<ZWaveNode*>::const_iterator it = devices.nodes.begin(); it != devices.nodes.end(); it++) {
			ZWaveNode *node = *it;
			printf("adding ago device for value id: %s - type: %s\n", node->getId().c_str(),node->getDevicetype().c_str());
			agoConnection->addDevice(node->getId().c_str(), node->getDevicetype().c_str());

		}*/

		agoConnection->run();
	} else {
		printf("unable to initialize OZW\n");
	}	
	Manager::Destroy();

	pthread_mutex_destroy( &g_criticalSection );
	return 0;
}

