/* soapRemoteDiscoveryBindingProxy.cpp
   Generated by gSOAP 2.8.15 from ./onvif/onvif.h

Copyright(C) 2000-2013, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under ONE of the following licenses:
GPL or Genivia's license for commercial use.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
*/

#include "soapRemoteDiscoveryBindingProxy.h"

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy()
{	RemoteDiscoveryBindingProxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy(const struct soap &_soap) : soap(_soap)
{ }

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy(const char *url)
{	RemoteDiscoveryBindingProxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
	soap_endpoint = url;
}

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy(soap_mode iomode)
{	RemoteDiscoveryBindingProxy_init(iomode, iomode);
}

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy(const char *url, soap_mode iomode)
{	RemoteDiscoveryBindingProxy_init(iomode, iomode);
	soap_endpoint = url;
}

RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy(soap_mode imode, soap_mode omode)
{	RemoteDiscoveryBindingProxy_init(imode, omode);
}

RemoteDiscoveryBindingProxy::~RemoteDiscoveryBindingProxy()
{ }

void RemoteDiscoveryBindingProxy::RemoteDiscoveryBindingProxy_init(soap_mode imode, soap_mode omode)
{	soap_imode(this, imode);
	soap_omode(this, omode);
	soap_endpoint = NULL;
	static const struct Namespace namespaces[] =
{
	{"SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope", "http://schemas.xmlsoap.org/soap/envelope/", NULL},
	{"SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding", "http://schemas.xmlsoap.org/soap/encoding/", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing", NULL, NULL},
	{"wsdd", "http://schemas.xmlsoap.org/ws/2005/04/discovery", NULL, NULL},
	{"chan", "http://schemas.microsoft.com/ws/2005/02/duplex", NULL, NULL},
	{"wsa5", "http://www.w3.org/2005/08/addressing", "http://schemas.xmlsoap.org/ws/2004/08/addressing", NULL},
	{"c14n", "http://www.w3.org/2001/10/xml-exc-c14n#", NULL, NULL},
	{"wsu", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd", NULL, NULL},
	{"xenc", "http://www.w3.org/2001/04/xmlenc#", NULL, NULL},
	{"wsc", "http://schemas.xmlsoap.org/ws/2005/02/sc", NULL, NULL},
	{"ds", "http://www.w3.org/2000/09/xmldsig#", NULL, NULL},
	{"wsse", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd", "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd", NULL},
	{"ns4", "http://www.onvif.org/ver10/pacs", NULL, NULL},
	{"xmime", "http://tempuri.org/xmime.xsd", NULL, NULL},
	{"xop", "http://www.w3.org/2004/08/xop/include", NULL, NULL},
	{"tt", "http://www.onvif.org/ver10/schema", NULL, NULL},
	{"wsrfbf", "http://docs.oasis-open.org/wsrf/bf-2", NULL, NULL},
	{"wstop", "http://docs.oasis-open.org/wsn/t-1", NULL, NULL},
	{"wsrfr", "http://docs.oasis-open.org/wsrf/r-2", NULL, NULL},
	{"ns1", "http://www.onvif.org/ver10/deviceIO/wsdl", NULL, NULL},
	{"ns2", "http://www.onvif.org/ver10/actionengine/wsdl", NULL, NULL},
	{"ns3", "http://www.onvif.org/ver10/accesscontrol/wsdl", NULL, NULL},
	{"ns5", "http://www.onvif.org/ver10/doorcontrol/wsdl", NULL, NULL},
	{"tad", "http://www.onvif.org/ver10/analyticsdevice/wsdl", NULL, NULL},
	{"tan", "http://www.onvif.org/ver20/analytics/wsdl", NULL, NULL},
	{"tdn", "http://www.onvif.org/ver10/network/wsdl", NULL, NULL},
	{"tds", "http://www.onvif.org/ver10/device/wsdl", NULL, NULL},
	{"tev", "http://www.onvif.org/ver10/events/wsdl", NULL, NULL},
	{"wsnt", "http://docs.oasis-open.org/wsn/b-2", NULL, NULL},
	{"timg", "http://www.onvif.org/ver20/imaging/wsdl", NULL, NULL},
	{"tls", "http://www.onvif.org/ver10/display/wsdl", NULL, NULL},
	{"tptz", "http://www.onvif.org/ver20/ptz/wsdl", NULL, NULL},
	{"trc", "http://www.onvif.org/ver10/recording/wsdl", NULL, NULL},
	{"trp", "http://www.onvif.org/ver10/replay/wsdl", NULL, NULL},
	{"trt", "http://www.onvif.org/ver10/media/wsdl", NULL, NULL},
	{"trv", "http://www.onvif.org/ver10/receiver/wsdl", NULL, NULL},
	{"tse", "http://www.onvif.org/ver10/search/wsdl", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	soap_set_namespaces(this, namespaces);
}

void RemoteDiscoveryBindingProxy::destroy()
{	soap_destroy(this);
	soap_end(this);
}

void RemoteDiscoveryBindingProxy::reset()
{	destroy();
	soap_done(this);
	soap_init(this);
	RemoteDiscoveryBindingProxy_init(SOAP_IO_DEFAULT, SOAP_IO_DEFAULT);
}

void RemoteDiscoveryBindingProxy::soap_noheader()
{	this->header = NULL;
}

void RemoteDiscoveryBindingProxy::soap_header(char *wsa__MessageID, struct wsa__Relationship *wsa__RelatesTo, struct wsa__EndpointReferenceType *wsa__From, struct wsa__EndpointReferenceType *wsa__ReplyTo, struct wsa__EndpointReferenceType *wsa__FaultTo, char *wsa__To, char *wsa__Action, struct wsdd__AppSequenceType *wsdd__AppSequence, char *wsa5__MessageID, struct wsa5__RelatesToType *wsa5__RelatesTo, struct wsa5__EndpointReferenceType *wsa5__From, struct wsa5__EndpointReferenceType *wsa5__ReplyTo, struct wsa5__EndpointReferenceType *wsa5__FaultTo, char *wsa5__To, char *wsa5__Action, struct chan__ChannelInstanceType *chan__ChannelInstance, struct _wsse__Security *wsse__Security)
{	::soap_header(this);
	this->header->wsa__MessageID = wsa__MessageID;
	this->header->wsa__RelatesTo = wsa__RelatesTo;
	this->header->wsa__From = wsa__From;
	this->header->wsa__ReplyTo = wsa__ReplyTo;
	this->header->wsa__FaultTo = wsa__FaultTo;
	this->header->wsa__To = wsa__To;
	this->header->wsa__Action = wsa__Action;
	this->header->wsdd__AppSequence = wsdd__AppSequence;
	this->header->wsa5__MessageID = wsa5__MessageID;
	this->header->wsa5__RelatesTo = wsa5__RelatesTo;
	this->header->wsa5__From = wsa5__From;
	this->header->wsa5__ReplyTo = wsa5__ReplyTo;
	this->header->wsa5__FaultTo = wsa5__FaultTo;
	this->header->wsa5__To = wsa5__To;
	this->header->wsa5__Action = wsa5__Action;
	this->header->chan__ChannelInstance = chan__ChannelInstance;
	this->header->wsse__Security = wsse__Security;
}

const SOAP_ENV__Header *RemoteDiscoveryBindingProxy::soap_header()
{	return this->header;
}

const SOAP_ENV__Fault *RemoteDiscoveryBindingProxy::soap_fault()
{	return this->fault;
}

const char *RemoteDiscoveryBindingProxy::soap_fault_string()
{	return *soap_faultstring(this);
}

const char *RemoteDiscoveryBindingProxy::soap_fault_detail()
{	return *soap_faultdetail(this);
}

int RemoteDiscoveryBindingProxy::soap_close_socket()
{	return soap_closesock(this);
}

int RemoteDiscoveryBindingProxy::soap_force_close_socket()
{	return soap_force_closesock(this);
}

void RemoteDiscoveryBindingProxy::soap_print_fault(FILE *fd)
{	::soap_print_fault(this, fd);
}

#ifndef WITH_LEAN
#ifndef WITH_COMPAT
void RemoteDiscoveryBindingProxy::soap_stream_fault(std::ostream& os)
{	::soap_stream_fault(this, os);
}
#endif

char *RemoteDiscoveryBindingProxy::soap_sprint_fault(char *buf, size_t len)
{	return ::soap_sprint_fault(this, buf, len);
}
#endif

int RemoteDiscoveryBindingProxy::Hello(const char *endpoint, const char *soap_action, struct wsdd__HelloType tdn__Hello, struct wsdd__ResolveType &tdn__HelloResponse)
{	struct soap *soap = this;
	struct __tdn__Hello soap_tmp___tdn__Hello;
	if (endpoint)
		soap_endpoint = endpoint;
	if (soap_action == NULL)
		soap_action = "http://www.onvif.org/ver10/network/wsdl/Hello";
	soap->encodingStyle = NULL;
	soap_tmp___tdn__Hello.tdn__Hello = tdn__Hello;
	soap_begin(soap);
	soap_set_version(soap, 2); /* SOAP1.2 */
	soap_serializeheader(soap);
	soap_serialize___tdn__Hello(soap, &soap_tmp___tdn__Hello);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___tdn__Hello(soap, &soap_tmp___tdn__Hello, "-tdn:Hello", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_url(soap, soap_endpoint, NULL), soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___tdn__Hello(soap, &soap_tmp___tdn__Hello, "-tdn:Hello", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!&tdn__HelloResponse)
		return soap_closesock(soap);
	soap_default_wsdd__ResolveType(soap, &tdn__HelloResponse);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get_wsdd__ResolveType(soap, &tdn__HelloResponse, "tdn:HelloResponse", "wsdd:ResolveType");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

int RemoteDiscoveryBindingProxy::Bye(const char *endpoint, const char *soap_action, struct wsdd__ByeType tdn__Bye, struct wsdd__ResolveType &tdn__ByeResponse)
{	struct soap *soap = this;
	struct __tdn__Bye soap_tmp___tdn__Bye;
	if (endpoint)
		soap_endpoint = endpoint;
	if (soap_action == NULL)
		soap_action = "http://www.onvif.org/ver10/network/wsdl/Bye";
	soap->encodingStyle = NULL;
	soap_tmp___tdn__Bye.tdn__Bye = tdn__Bye;
	soap_begin(soap);
	soap_set_version(soap, 2); /* SOAP1.2 */
	soap_serializeheader(soap);
	soap_serialize___tdn__Bye(soap, &soap_tmp___tdn__Bye);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___tdn__Bye(soap, &soap_tmp___tdn__Bye, "-tdn:Bye", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_url(soap, soap_endpoint, NULL), soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___tdn__Bye(soap, &soap_tmp___tdn__Bye, "-tdn:Bye", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!&tdn__ByeResponse)
		return soap_closesock(soap);
	soap_default_wsdd__ResolveType(soap, &tdn__ByeResponse);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get_wsdd__ResolveType(soap, &tdn__ByeResponse, "tdn:ByeResponse", "wsdd:ResolveType");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}

int RemoteDiscoveryBindingProxy::Probe(const char *endpoint, const char *soap_action, struct wsdd__ProbeType tdn__Probe, struct wsdd__ProbeMatchesType &tdn__ProbeResponse)
{	struct soap *soap = this;
	struct __tdn__Probe soap_tmp___tdn__Probe;
	if (endpoint)
		soap_endpoint = endpoint;
	if (soap_action == NULL)
		soap_action = "http://www.onvif.org/ver10/network/wsdl/Probe";
	soap->encodingStyle = NULL;
	soap_tmp___tdn__Probe.tdn__Probe = tdn__Probe;
	soap_begin(soap);
	soap_set_version(soap, 2); /* SOAP1.2 */
	soap_serializeheader(soap);
	soap_serialize___tdn__Probe(soap, &soap_tmp___tdn__Probe);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___tdn__Probe(soap, &soap_tmp___tdn__Probe, "-tdn:Probe", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_url(soap, soap_endpoint, NULL), soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___tdn__Probe(soap, &soap_tmp___tdn__Probe, "-tdn:Probe", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	if (!&tdn__ProbeResponse)
		return soap_closesock(soap);
	soap_default_wsdd__ProbeMatchesType(soap, &tdn__ProbeResponse);
	if (soap_begin_recv(soap)
	 || soap_envelope_begin_in(soap)
	 || soap_recv_header(soap)
	 || soap_body_begin_in(soap))
		return soap_closesock(soap);
	soap_get_wsdd__ProbeMatchesType(soap, &tdn__ProbeResponse, "tdn:ProbeResponse", "wsdd:ProbeMatchesType");
	if (soap->error)
		return soap_recv_fault(soap, 0);
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap_closesock(soap);
	return soap_closesock(soap);
}
/* End of client proxy code */
