#ifndef ConcreteModerationStrategyExample_HPP
#define ConcreteModerationStrategyExample_HPP

#include "ConcreteModerationStrategyExample.hpp"
#include "ModerationStrategy.hpp"
#include "Moderator.hpp"

namespace SpacePosts
{
    /**
     * @brief Example for what a concrete implementation of the ModerationStrategy interface could look like
     * 
     * This is just an example. It does not implement any moderation criteria. 
     */
    class ConcreteModerationStrategyExample : ModerationStrategy
    {
        public:
            virtual bool checkMessage(
                const SpacePosts::SpacePost &message /*!< the SpacePost to check */
            ) override;
    };
}

#endif