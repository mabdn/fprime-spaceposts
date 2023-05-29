// ======================================================================
// \title  Transceiver.cpp
// \author Marius Baden
// \brief  cpp file for Transceiver component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <SpacePosts/Transceiver/Transceiver.hpp>
#include <config/TransceiverCfg.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include "Transceiver.hpp"

namespace SpacePosts
{

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  Transceiver ::
      Transceiver(
          const char *const compName) : TransceiverComponentBase(compName)
  {
  }

  void Transceiver ::
      init(
          const NATIVE_INT_TYPE instance)
  {
    TransceiverComponentBase::init(instance);
  }

  Transceiver ::
      ~Transceiver()
  {
  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void Transceiver ::
      scheduleDownlink_handler(
          const NATIVE_INT_TYPE portNum,
          NATIVE_UINT_TYPE context)
  {
    this->sendMessages();
  }

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------

  void Transceiver ::
      STORE_MESSAGE_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq,
          SpacePosts::SpacePost msg)
  {
    const SpacePosts::MessageStorageStatus status = this->storeMessage_out(0, &msg);
    const Fw::CmdResponse response = status == SpacePosts::MessageStorageStatus::OK
                                         ? Fw::CmdResponse::OK
                                         : Fw::CmdResponse::EXECUTION_ERROR;
    this->cmdResponse_out(opCode, cmdSeq, response);
  }

  void Transceiver ::
      DOWNLINK_LAST_MESSAGES_GDS_cmdHandler(
          const FwOpcodeType opCode,
          const U32 cmdSeq)
  {
    const bool success = this->sendMessages();
    const Fw::CmdResponse response = success ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR;
    this->cmdResponse_out(opCode, cmdSeq, response);
  }

  void Transceiver::DOWNLINK_LAST_MESSAGES_HAMUSER_cmdHandler(
      const FwOpcodeType opCode,
      const U32 cmdSeq)
  {
    // Check if HAM user command is enabled
    Fw::ParamValid valid;
    const bool enabled = paramGet_ALLOW_HAMUSER_DOWNLINK_CMD(valid);
    FW_ASSERT(valid.e == Fw::ParamValid::VALID || valid.e == Fw::ParamValid::DEFAULT, valid.e);

    if (!enabled)
    {
      this->rejectHamUserDownlinkCmd(opCode, cmdSeq);
      return;
    }

    // Check if cooldown time has passed
    const U32 cooldown_time = paramGet_DOWNLINK_COOLDOWN_TIME(valid);
    FW_ASSERT(valid.e == Fw::ParamValid::VALID || valid.e == Fw::ParamValid::DEFAULT, valid.e);

    const Fw::Time currentTime = this->getTime();
    const Fw::Time timeSinceLastDownlink = Fw::Time::sub(currentTime, this->m_lastDownlinkTime);
    if (timeSinceLastDownlink.getSeconds() < cooldown_time)
    {
      this->rejectHamUserDownlinkCmd(opCode, cmdSeq);
      return;
    }

    // After enabled check, handle just as GDS request
    this->DOWNLINK_LAST_MESSAGES_GDS_cmdHandler(opCode, cmdSeq);
  }

  // ----------------------------------------------------------------------
  // Private Component Methods
  // ----------------------------------------------------------------------

  bool Transceiver::
      sendMessages()
  {
    SpacePost_Batch messages{};
    this->loadMessages_out(0, TRANSCEIVER_NUM_MESSAGES_TO_DOWNLINK, messages);
    const SpacePost_Array &message_array = messages.getmessages();

    if (messages.getnumValidMessages() <= 0)
    {
      // No error event in this case.
      // Message storage will have triggered error events already if messages existed but loading failed.
      return false;
    }

    for (U32 i = 0; i < messages.getnumValidMessages(); i++)
    {
      const SpacePost &message = message_array[i];

      Fw::ComBuffer comBuffer{}; // ComBuffer is autoamtically allocated on the stack on intialization => no alloc call

      // Serialize message into comBuffer
      comBuffer.resetSer();
      Fw::SerializeStatus status = comBuffer.serialize(message);
      FW_ASSERT(Fw::FW_SERIALIZE_OK == status, static_cast<NATIVE_INT_TYPE>(status));

      if (this->isConnected_downlinkMessage_OutputPort(0))
      {
        // Svc.Framer does not use the context parameter, so we can just put 0
        this->downlinkMessage_out(0, comBuffer, 0);
      }
    }

    this->m_lastDownlinkTime = this->getTime();
    return true;
  }

  void Transceiver::rejectHamUserDownlinkCmd(const FwOpcodeType opCode, const U32 cmdSeq)
  {
    // Give a response for now. This behavior might change in the future, thus it is its own function.
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
  }

} // end namespace SpacePosts
