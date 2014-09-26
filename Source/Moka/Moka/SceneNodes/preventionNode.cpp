#include "preventionNode.h"
#include "../GameObjects/preventionObject.h"

#include "Trambo/Events/event.h"

#include <SFML/Window/Mouse.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>


PreventionNode::PreventionNode(sf::RenderWindow &window, const sf::View &view, const PreventionObject &preventionObject)
: mWindow(window)
, mView(view)
, mPreventionObject(preventionObject)
{
	mHightlight.setSize(static_cast<sf::Vector2f>(sf::Vector2i(mPreventionObject.getWidth(), mPreventionObject.getHeight())));
	mHightlight.setPosition(static_cast<sf::Vector2f>(sf::Vector2i(mPreventionObject.getX(), mPreventionObject.getY())));
}

void PreventionNode::updateSelection()
{
	const sf::Vector2i relativeToWindow = sf::Mouse::getPosition(mWindow);
	const sf::Vector2f relativeToWorld = mWindow.mapPixelToCoords(relativeToWindow, mView);
	const sf::Vector2f MousePosition(relativeToWorld.x, relativeToWorld.y);
	const sf::FloatRect PrevObjRect(static_cast<sf::FloatRect>(sf::IntRect(mPreventionObject.getX(),
		mPreventionObject.getY(), mPreventionObject.getWidth(), mPreventionObject.getHeight())));

	if (PrevObjRect.contains(MousePosition))
		mIsSelected = true;
	else
		mIsSelected = false;
}


