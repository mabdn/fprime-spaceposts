#ifndef ModerationStrategy_HPP
#define ModerationStrategy_HPP

namespace SpacePosts
{
    /**
     * @brief SpacePost moderation strategy which encapsulates the moderation criteria and their implementation
     * 
     * By injecting different implementations of the interface into the Moderator component, 
     * developers can realize different moderation criteria. Hence, the code of the Moderator component 
     * does not change if the moderation criteria M-MOD-* change.
     */
    class ModerationStrategy
    {
        public:

            /**
             * @brief Destroys the ModerationStrategy object
             * 
             * Virtual destructor for polymorphic destruction.
             */
            virtual ~ModerationStrategy() {}

            /**
             * @brief Decides whether a SpacePost is acceptable or inappropriate
             * 
             * An implementation must check the SpacePost a set of concrete moderation criteria as defined
             * by the requirements M-MOD-* in the Moderator component software design documentation.
             * 
             * @param message the SpacePost to check
             * @return true if the message is acceptable, false if it is inappropriate
             */
            virtual bool checkMessage(
                const SpacePosts::SpacePost &message /*!< the SpacePost to check */
            ) = 0;
    };
    
}

#endif