// ======================================================================
// \title  Transceiver.hpp
// \author Marius Baden
// \brief  hpp file for Transceiver component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef TRANSCEIVER_HPP
#define TRANSCEIVER_HPP

#include "SpacePosts/Transceiver/TransceiverComponentAc.hpp"

namespace SpacePosts
{

    class Transceiver : public TransceiverComponentBase
    {
        PRIVATE :
            // ----------------------------------------------------------------------
            // Component Attributes
            // ----------------------------------------------------------------------

            //! The time of the last performed downlink
            //!
            //! No matter if the downlink was successful or not
            //! No matter how the downlink was triggered (GDS command, HAM user command, schedule port)
            Fw::Time m_lastDownlinkTime{};

    public:
        // ----------------------------------------------------------------------
        // Construction, initialization, and destruction
        // ----------------------------------------------------------------------

        //! Construct object Transceiver
        //!
        Transceiver(
            const char *const compName /*!< The component name*/
        );

        //! Initialize object Transceiver
        //!
        void init(
            const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

        //! Destroy object Transceiver
        //!
        ~Transceiver();

        PRIVATE :

            // ----------------------------------------------------------------------
            // Handler implementations for user-defined typed input ports
            // ----------------------------------------------------------------------

            //! Handler implementation for scheduleDownlink
            //!
            void
            scheduleDownlink_handler(
                const NATIVE_INT_TYPE portNum, /*!< The port number*/
                NATIVE_UINT_TYPE context       /*!< The call order */
                ) override;

        PRIVATE :

            // ----------------------------------------------------------------------
            // Command handler implementations
            // ----------------------------------------------------------------------

            //! Implementation for STORE_MESSAGE command handler
            //! Store the given SpacePosts on the satellite
            void
            STORE_MESSAGE_cmdHandler(
                const FwOpcodeType opCode, /*!< The opcode*/
                const U32 cmdSeq,          /*!< The command sequence number*/
                SpacePost msg        /*!<
                       The message to store on the satellite
                       */
                ) override;

        //! Implementation for DOWNLINK_LAST_MESSAGES command handler
        //!
        void DOWNLINK_LAST_MESSAGES_GDS_cmdHandler(
            const FwOpcodeType opCode, /*!< The opcode*/
            const U32 cmdSeq           /*!< The command sequence number*/
            ) override;

        //! Implementation for DOWNLINK_LAST_MESSAGES command handler
        //!
        void DOWNLINK_LAST_MESSAGES_HAMUSER_cmdHandler(
            const FwOpcodeType opCode, /*!< The opcode*/
            const U32 cmdSeq           /*!< The command sequence number*/
            ) override;

        PRIVATE :

            // ----------------------------------------------------------------------
            // Private Component Methods
            // ----------------------------------------------------------------------

            /**
             * @brief Load a certain number of last stored SpacePosts from the MessageStorage and initiate their
             * downlink by
             * sending them to a Svc.Framer component
             *
             * @return false iff no downlink was successfully initiated
             * @return true iff at least one downlink was successfully initiated
             */
            bool
            sendMessages();

        /**
         * @brief Encapsulates what to do when a HAM user's command to downlink the last stored SpacePosts
         * has to be rejected
         *
         * For example, this encapsulates whether or not to send a cmd response and what kinds of events and
         * telemetry to emit.
         *
         * @param opCode The opcode
         * @param cmdSeq The command sequence number
         */
        void rejectHamUserDownlinkCmd(const FwOpcodeType opCode, const U32 cmdSeq);
    };

} // end namespace SpacePosts

#endif
