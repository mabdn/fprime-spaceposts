#include "ConcreteModerationStrategyExample.hpp"

namespace SpacePosts
{
    bool ConcreteModerationStrategyExample::checkMessage(
        const SpacePosts::SpacePost &message)
    {
        bool result{true};

        // TODO Implement depending on your application: Check message against moderation criteria here

        return result;
    }
}
