#include "ProxyPlugin.h"
#include <cassert>
#include <utl/UtlString.h>
#include <sys/time.h>
#include <sstream>
#include <net/NetBase64Codec.h>
#include <sqa/StateQueueMessage.h>

/// Factory used by PluginHooks to dynamically link the plugin instance
extern "C" SipBidirectionalProcessorPlugin* getTransactionPlugin(const UtlString& pluginName)
{
   return new HomerProxyPlugin(pluginName);
}

HomerProxyPlugin::HomerProxyPlugin(const UtlString& instanceName, int priority) :
  SipBidirectionalProcessorPlugin(instanceName, priority),
  _sqa("HomerProxyPlugin", 1),
  _localPort(0)
{
}

HomerProxyPlugin::~HomerProxyPlugin()
{
}

void HomerProxyPlugin::readConfig(OsConfigDb& configDb)
{
}

void HomerProxyPlugin::initialize()
{
  assert(_pUserAgent);
  UtlString host;
  int port;
  if (_pUserAgent->getLocalAddress(&host, &port))
  {
    _localHost = host.data();
    _localPort = port;
  }
}

void HomerProxyPlugin::handleIncoming(SipMessage& message, const char* address, int port)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  ssize_t length = 0;
  UtlString bufferString;
  message.getBytes(&bufferString, &length, true) ;

  if (length > 65536)
    return;

  std::string data(bufferString.data(), length);

  StateQueueMessage msg;
  msg.setType(StateQueueMessage::Data);
  msg.set("Outgoing", 0);
  msg.set("IpProtoId", (int)HEPMessage::TCP);
  msg.set("Ip4SrcAddress", address);
  msg.set("Ip4DestAddress", _localHost.c_str());
  msg.set("SrcPort", port);
  msg.set("DestPort", _localPort);
  msg.set("TimeStamp", (double)now.tv_sec);
  msg.set("TimeStampMicroOffset", (double)now.tv_usec);
  msg.set("Data", data.c_str());


  std::string msgData = msg.data();
  _sqa.publish("CAP", msgData.c_str());
}

void HomerProxyPlugin::handleOutgoing(SipMessage& message, const char* address, int port)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  
  ssize_t length = 0;
  UtlString bufferString;
  message.getBytes(&bufferString, &length, true);
  
  if (length > 65536)
    return;
  
  std::string data(bufferString.data(), length);


  StateQueueMessage msg;
  msg.setType(StateQueueMessage::Data);
  msg.set("Outgoing", 1);
  msg.set("IpProtoId", (int)HEPMessage::TCP);
  msg.set("Ip4SrcAddress", _localHost.c_str());
  msg.set("Ip4DestAddress", address);
  msg.set("SrcPort", _localPort);
  msg.set("DestPort", port);
  msg.set("TimeStamp", (double)now.tv_sec);
  msg.set("TimeStampMicroOffset", (double)now.tv_usec);
  msg.set("Data", data.c_str());

  std::string msgData = msg.data();
  _sqa.publish("CAP", msgData.c_str());
}


