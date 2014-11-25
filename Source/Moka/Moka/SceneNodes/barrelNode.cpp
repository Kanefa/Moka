#include "barrelNode.h"
#include "../GameObjects/interactiveObject.h"
#include "../HUD/chatBox.h"
#include "../HUD/optionsUI.h"
#include "../Levels/uiBundle.h"
#include "../Resources/resourceIdentifiers.h"

#include "Trambo/Events/event.h"
#include "Trambo/Localize/localize.h"
#include "Trambo/Sounds/soundPlayer.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>


BarrelNode::BarrelNode(const InteractiveObject &interactiveObject, const sf::RenderWindow &window, const sf::View &view
	, UIBundle &uiBundle, trmb::SoundPlayer &soundPlayer, ChatBox &chatBox)
: PreventionNode(interactiveObject, window, view, uiBundle)
, mLeftClickPress(0x6955d309)
, mSoundPlayer(soundPlayer)
, mChatBox(chatBox)
{
	mCallbackPairs.emplace_back(CallbackPair(std::bind(&BarrelNode::addLid, this), std::bind(&BarrelNode::undoLid, this)));
	mUIElemStates.emplace_back(true);
}

void BarrelNode::handleEvent(const trmb::Event &gameEvent)
{
	InteractiveNode::handleEvent(gameEvent);

	if (!mDisableInput)
	{
		if (mLeftClickPress == gameEvent.getType())
		{
			mPreviousSelectedState = mSelected;
			if (mSelected && !isMouseOverUI(mUIBundle.getBarrelUI().getRect()))
			{
				mSelected = false;
			}

			if (isMouseOverObject()
				&& !isMouseOverUI(mUIBundle.getBarrelUI().getRect())
				&& !isMouseOverUI(mUIBundle.getWindowUI().getRect())
				&& !isMouseOverUI(mUIBundle.getClinicUI().getRect())
				&& !isMouseOverUI(mUIBundle.getHouseUI().getRect()))
			{
				mSelected = true;
				if (!mPreviousSelectedState)
					activate();
			}
		}
	}
}

void BarrelNode::drawCurrent(sf::RenderTarget &target, sf::RenderStates states) const
{
	if (mSelected)
	{
		target.draw(mHightlight, states);
		target.draw(mUIBundle.getBarrelUI(), states);
	}
}

void BarrelNode::updateCurrent(sf::Time)
{
	// ALW - Do not apply the InteractiveNode's transform, because there are multiple instances that share a single UndoUI. If the 
	// ALW - transform is applied then the UndoUI would be at the location specified by the translation of multiple InteractiveNodes
	// ALW - which is not correct. The workaround is to let the InteractiveNode's position default to 0.0f, 0.0f and then use the
	// ALW - InteractiveObjects coordinates to position the UndoUI in the world. This way the InteractiveNode's transform does not
	// ALW - need to be applied to the UndoUI. To keep the handler interface consistent the Identity transform is passed in and applied.
	sf::Transform transform = sf::Transform::Identity;

	mUIBundle.getBarrelUI().handler(mWindow, mView, transform);
}

void BarrelNode::activate()
{
	updateUndoUI();	
	mSoundPlayer.play(SoundEffects::ID::Button);
	// ALW - ChatBox::UpdateText() can generate a mCreatePrompt event when an interactive object
	// ALW - is selected. This asynchronous event will force InteractiveNode classes to ignore
	// ALW - left and right click events. Then if <enter> is pressed an mEnter event will be
	// ALW - generated allowing InteractiveNode classes to handle left and right click events.
	// ALW - However, if another interactive object is selected that occurs before the currently
	// ALW - selected interactive object in the SceneNode then this newly selected interactive
	// ALW - object will cause ChatBox::UpdateText() to be called which generates a mCreatePrompt.
	// ALW - InteractiveNodes are then forced to ignore left and right click events, so the original
	// ALW - interactive object will be left selected. To remedy this all InteractiveNodes deselect
	// ALW - themselves when a mCreatePrompt is generated. Immediately afterwards the InteractiveNode
	// ALW - that generated the mCreatePrompt is reselected.
	mChatBox.updateText(trmb::Localize::getInstance().getString("inspectBarrel"));
	mSelected = true;
}

void BarrelNode::updateUndoUI()
{
	const float verticalBuffer = 10.0f;
	mUIBundle.getBarrelUI().setPosition(sf::Vector2f(mInteractiveObject.getX() + mInteractiveObject.getWidth() / 2.0f
		, mInteractiveObject.getY() + mInteractiveObject.getHeight() + verticalBuffer));

	mUIBundle.getBarrelUI().setCallbacks(mCallbackPairs);

	mUIBundle.getBarrelUI().setUIElemState(mUIElemStates);
}

void BarrelNode::addLid()
{
	mUIElemStates.front() = false;
	// ALW - TODO - Add lid
}

void BarrelNode::undoLid()
{
	mUIElemStates.front() = true;
	// ALW - TODO - Remove lid
}
