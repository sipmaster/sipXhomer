// Copyright (c) 2012 eZuce, Inc. All rights reserved.
// Contributed to SIPfoundry under a Contributor Agreement
//
// This software is free software; you can redistribute it and/or modify it under
// the terms of the Affero General Public License (AGPL) as published by the
// Free Software Foundation; either version 3 of the License, or (at your option)
// any later version.
//
// This software is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
// details.

#include <net/NetBase64Codec.h>
#include "os/OsLogger.h"
#include "sqa/ServiceOptions.h"
#include "sqa/sqaclient.h"
#include <boost/date_time.hpp>
#include "sipxhomer/HEPCaptureAgent.h"
#include "sqa/StateQueueMessage.h"


using namespace resip;

HEPCaptureAgent::HEPCaptureAgent(ServiceOptions& options, HEPDao& dao) :
  _options(options),
  _pWatcher(0),
  _dao(dao),
  _pRunThread(0)
{
  OS_LOG_ERROR(FAC_NET, "HEPCaptureAgent CREATED");
}

HEPCaptureAgent::~HEPCaptureAgent()
{
  stop();
}

void HEPCaptureAgent::run()
{
  if (_pRunThread)
    return;
  _isRunning = true;
  _pRunThread = new boost::thread(boost::bind(&HEPCaptureAgent::internalRun, this));
}

void HEPCaptureAgent::internalRun()
{
  if (_pWatcher)
    return;
  
  std::string sqaAddress;
  std::string sqaPort;
  
 
  _pWatcher = new SQAWatcher("HEPCaptureAgent", "CAP", 1);

  OS_LOG_INFO(FAC_NET, "HEPCaptureAgent::internalRun started capturing events");
  while(_isRunning)
  {
    SQAEvent* pEvent = _pWatcher->watch();
    if (pEvent)
    {
      try
      {

        std::string buff = std::string(pEvent->data, pEvent->data_len);
        StateQueueMessage object;
        if (object.parseData(buff))
          _dao.save(object);
      }
      catch(std::exception& error)
      {
      }
      delete pEvent;
    }
    else
    {
      _isRunning = false;
      break;
    }
  }

  delete _pWatcher;
  _pWatcher = 0;
}

void HEPCaptureAgent::stop()
{
  _isRunning = false;
  if (_pRunThread)
  {
    _pRunThread->join();
    delete _pRunThread;
    _pRunThread = 0;
  }
}

