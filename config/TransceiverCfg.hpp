/*
 * TransceiverCfg.hpp
 *
 *  Created on: May 14, 2023
 *      Author: Marius Baden
 */

#ifndef Transceiver_TransceiverCfg_HPP_
#define Transceiver_TransceiverCfg_HPP_

#include "SpacePosts/MessageTypes/FppConstantsAc.hpp"


// Anonymous namespace for configuration parameters
namespace
{

  enum
  { 
    // The number of SpacePosts to downlink whenever a downlink is requested via command or a downlink
    // is triggered by the downlink schedule input port.
    //
    // This is the configurable number of messages to downlink as required in NF-TRA-010 of the Transceiver's 
    // software design document.
    //
    // In [1, SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size].
    //
    // Currently, defined via the maximum number of messages a SpacePost_Batch can hold 
    // (= SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size). Thus, it is the maximum
    // number of messages which can be loaded at once. A number higher than this does not work without further 
    // refactoring!
    TRANSCEIVER_NUM_MESSAGES_TO_DOWNLINK = SpacePosts::FppConstant_SpacePost_Batch_Size::SpacePost_Batch_Size,
  };

}

#endif /* MessageStorage_MessageStorageCfg_HPP_ */
