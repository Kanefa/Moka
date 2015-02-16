#include "world.h"
#include "../SceneNodes/barrelNode.h"
#include "../SceneNodes/barrelUpdateNode.h"
#include "../SceneNodes/barrelUINode.h"
#include "../SceneNodes/darkness.h"
#include "../SceneNodes/doorNode.h"
#include "../SceneNodes/doorUINode.h"
#include "../SceneNodes/doorUpdateNode.h"
#include "../SceneNodes/windowNode.h"
#include "../SceneNodes/windowUINode.h"
#include "../SceneNodes/windowUpdateNode.h"
#include "../SceneNodes/clinicNode.h"
#include "../SceneNodes/clinicUINode.h"
#include "../SceneNodes/clinicUpdateNode.h"
#include "../SceneNodes/heroNode.h"
#include "../SceneNodes/houseNode.h"
#include "../SceneNodes/houseUINode.h"
#include "../SceneNodes/houseUpdateNode.h"
#include "../SceneNodes/mosquitoNode.h"
#include "../GameObjects/interactiveObject.h"
#include "../Resources/resourceIdentifiers.h"

#include "Trambo/Events/event.h"
#include "Trambo/Localize/localize.h"
#include "Trambo/SceneNodes/mapLayerNode.h"
#include "Trambo/SceneNodes/spriteNode.h"
#include "Trambo/Sounds/soundPlayer.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <algorithm>
#include <cassert>


World::World(sf::RenderWindow& window, trmb::FontHolder& fonts, trmb::SoundPlayer& soundPlayer)
: mBeginSimulationEvent(0x5000e550)
, mFullscreen(0x5a0d2314)
, mWindowed(0x11e3c735)
, mWindow(window)
, mTarget(window)
, mFonts(fonts)
, mSoundPlayer(soundPlayer)
, mTextures()
, mSceneGraph()
, mSceneLayers()
, mObjectGroups("Data/Maps/World.tmx")
, mMap("Data/Maps/World.tmx")
, mWorldBounds(0.f, 0.f, static_cast<float>(mMap.getWidth() * mMap.getTileWidth()), static_cast<float>(mMap.getHeight() * mMap.getTileHeight()))
, mCamera(window.getDefaultView(), mWorldBounds)
, mHeroPosition(mWorldBounds.width / 2.f, mWorldBounds.height / 2.f)
, mHero(nullptr)
, mUIBundle(mChatBoxUI, mDaylightUI, mBarrelUI, mDoorUI, mWindowUI, mClinicUI, mHouseUI)
, mChatBoxUI(window, mCamera, Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, mUIBundle)
, mDaylightUI(window, mCamera, Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, mUIBundle, 0x6955d309, 0x128b8b25)
, mMainTrackerUI(window, Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer)
, mBarrelUI(Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, 0x6955d309, 0x128b8b25)
, mDoorUI(Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, 0x6955d309, 0x128b8b25)
, mWindowUI(Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, 0x6955d309, 0x128b8b25)
, mClinicUI(Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, 0x6955d309, 0x128b8b25)
, mHouseUI(Fonts::ID::Main, fonts, SoundEffects::ID::Button, soundPlayer, 0x6955d309, 0x128b8b25)
, mSpawnPositions()
, mRandomDevice()											// ALW - obtain random number from hardware
, mGenerator(mRandomDevice())								// ALW - Seed the generator
, mDistribution(0, mMap.getWidth() * mMap.getHeight() - 1)	// ALW - Define the range
, mBeginSimulationMode(false)
, mTotalCollisionTime(sf::seconds(1.0))
, mUpdateCollisionTime()
, mDoorToHouse()
, mWindowToHouse()
{
	mTextures.load(Textures::ID::Tiles, "Data/Textures/Tiles.png");
	mTextures.load(Textures::ID::MosquitoAnimation, "Data/Textures/mosquitoAnimation.png");

	generateSpawnPositions();
	buildScene();
	configureUIs();

	initializeDoorToHouseMap();
	initializeWindowToHouseMap();
}

void World::update(sf::Time dt)
{
	mSceneGraph.update(dt);					// ALW - Update the hero along with the rest of the scene graph
	mCamera.update(mHero->getPosition());	// ALW - Update the camera position

	updateCollisions(dt);

	updateSoundPlayer();
	mChatBoxUI.handler();
	mDaylightUI.handler();
}

void World::handleEvent(const trmb::Event &gameEvent)
{
	// ALW - Currently, fullscreen and windowed mode are the same.
	if (mFullscreen == gameEvent.getType() || mWindowed == gameEvent.getType())
	{
		mCamera.setSize(mTarget.getDefaultView().getSize());

		// ALW - Manually correct position of camera, necessary when pause menu is active.
		mCamera.update(mHero->getPosition());
	}
	else if (mBeginSimulationEvent == gameEvent.getType())
	{
		mBeginSimulationMode = true;
	}
}

void World::draw()
{
	mTarget.setView(mCamera.getView());
	mTarget.draw(mSceneGraph);
	mTarget.draw(mChatBoxUI);
	mTarget.draw(mDaylightUI);
	mTarget.draw(mMainTrackerUI);
}

void World::initializeDoorToHouseMap()
{
	std::set<trmb::SceneNode::Pair> collisionPairs;
	mSceneLayers[DoorSelection]->checkSceneCollision(*mSceneLayers[HouseSelection], collisionPairs, true);

	for (const auto &pair : collisionPairs)
	{
		assert(("Dynamic cast failed.", dynamic_cast<DoorNode *>(pair.first) != nullptr));
		assert(("Dynamic cast failed.", dynamic_cast<HouseNode *>(pair.second) != nullptr));

		mDoorToHouse[static_cast<DoorNode *>(pair.first)] = static_cast<HouseNode *>(pair.second);
	}
}

void World::initializeWindowToHouseMap()
{
	std::set<trmb::SceneNode::Pair> collisionPairs;
	mSceneLayers[WindowSelection]->checkSceneCollision(*mSceneLayers[HouseSelection], collisionPairs, true);

	for (const auto &pair : collisionPairs)
	{
		assert(("Dynamic cast failed.", dynamic_cast<WindowNode *>(pair.first) != nullptr));
		assert(("Dynamic cast failed.", dynamic_cast<HouseNode *>(pair.second) != nullptr));

		mWindowToHouse[static_cast<WindowNode *>(pair.first)] = static_cast<HouseNode *>(pair.second);
	}
}

void World::updateCollisions(sf::Time dt)
{
	mUpdateCollisionTime += dt;

	if (mBeginSimulationMode && mUpdateCollisionTime >= mTotalCollisionTime)
	{
		mUpdateCollisionTime -= mTotalCollisionTime;

		mosquitoDoorCollisions();
		mosquitoWindowCollisions();
	}
}

void World::mosquitoDoorCollisions()
{
	std::set<trmb::SceneNode::Pair> collisionPairs;
	mSceneLayers[Mosquitoes]->checkSceneCollision(*mSceneLayers[DoorSelection], collisionPairs, true);

	for (auto &pair : collisionPairs)
	{
		assert(("Dynamic cast failed.", dynamic_cast<MosquitoNode *>(pair.first) != nullptr));
		MosquitoNode * const mosquito = static_cast<MosquitoNode *>(pair.first);

		assert(("Dynamic cast failed.", dynamic_cast<DoorNode *>(pair.second) != nullptr));
		DoorNode * const door = static_cast<DoorNode *>(pair.second);
		assert(("The door key does not exist!", mDoorToHouse.count(door)));
		std::map<DoorNode *, HouseNode *>::const_iterator found = mDoorToHouse.find(door);
		HouseNode * const house = found->second;

		if (mosquito->isIndoor())
		{
			// ALW - Mosquito exits house
			const float tileHeight = 64.0f;
			const sf::Vector2f position = door->getPosition() + sf::Vector2f(0, tileHeight);
			mosquito->setPosition(position); // ALW - Move mosquito one tile below the door
			mosquito->setIndoor(false);
			house->subtractMosquito();
		}
		else
		{
			// ALW - Mosquito enters house
			mosquito->setPosition(house->getPosition());
			mosquito->setIndoor(true);
			house->addMosquito();
		}
	}
}

void World::mosquitoWindowCollisions()
{
	std::set<trmb::SceneNode::Pair> collisionPairs;
	mSceneLayers[Mosquitoes]->checkSceneCollision(*mSceneLayers[WindowSelection], collisionPairs, true);

	for (auto &pair : collisionPairs)
	{
		assert(("Dynamic cast failed.", dynamic_cast<MosquitoNode *>(pair.first) != nullptr));
		MosquitoNode * const mosquito = static_cast<MosquitoNode *>(pair.first);

		assert(("Dynamic cast failed.", dynamic_cast<WindowNode *>(pair.second) != nullptr));
		WindowNode * const window = static_cast<WindowNode *>(pair.second);
		assert(("The window key does not exist!", mWindowToHouse.count(window)));
		std::map<WindowNode *, HouseNode *>::const_iterator found = mWindowToHouse.find(window);
		HouseNode * const house = found->second;

		if (mosquito->isIndoor())
		{
			// ALW - Mosquito exits house
			const float tileHeight = 64.0f;
			const sf::Vector2f position = window->getPosition() - sf::Vector2f(0, tileHeight);
			mosquito->setPosition(position); // ALW - Move mosquito one tile above the window
			mosquito->setIndoor(false);
			house->subtractMosquito();
		}
		else
		{
			// ALW - Mosquito enters house
			mosquito->setPosition(house->getPosition());
			mosquito->setIndoor(true);
			house->addMosquito();
		}
	}
}

void World::updateSoundPlayer()
{
	// ALW - Delete sound effects that have finished playing.
	mSoundPlayer.removeStoppedSounds();
}

void World::configureUIs()
{
	// ALW - Must occur after World::buildScene(), otherwise the Nodes will miss the mCreateTextPrompt event.
	mChatBoxUI.updateText(trmb::Localize::getInstance().getString("greeting"));

	const unsigned int characterSize = 13;

	mBarrelUI.addUIElem("Cover", "Undo");
	mBarrelUI.setSize(sf::Vector2f(75.0f, 20.0f));
	mBarrelUI.setCharacterSize(characterSize);
	centerOrigin(mBarrelUI, true, false);
	mBarrelUI.hide();

	mDoorUI.addUIElem("Close", "Undo");
	mDoorUI.setSize(sf::Vector2f(75.0f, 20.0f));
	mDoorUI.setCharacterSize(characterSize);
	centerOrigin(mDoorUI, true, false);
	mDoorUI.hide();

	mWindowUI.addUIElem("Screen", "Undo");
	mWindowUI.addUIElem("Close", "Undo");
	mWindowUI.setSize(sf::Vector2f(75.0f, 20.0f));
	mWindowUI.setCharacterSize(characterSize);
	centerOrigin(mWindowUI, true, false);
	mWindowUI.hide();

	mClinicUI.setTabSize(sf::Vector2f(75.0f, 20.0f));
	mClinicUI.setLHSTabText("RDTs");
	mClinicUI.setRHSTabText("ACTs");
	mClinicUI.setCharacterSize(characterSize);
	centerOrigin(mClinicUI, true, false);
	mClinicUI.hide();

	mHouseUI.setTabSize(sf::Vector2f(75.0f, 20.0f));
	mHouseUI.setLHSTabText("Bed Net");
	mHouseUI.setRHSTabText("Repair");
	mHouseUI.setCharacterSize(characterSize);
	centerOrigin(mHouseUI, true, false);
	mHouseUI.hide();

	mMainTrackerUI.setTotalMosquito(500);
	mMainTrackerUI.setTotalResident(30);
}

void World::buildScene()
{
	// Initialize the different layers
	for (std::size_t i = 0; i < LayerCount; ++i)
	{
		trmb::SceneNode::Ptr layer(new trmb::SceneNode());
		mSceneLayers[i] = layer.get();

		mSceneGraph.attachChild(std::move(layer));
	}

	// Add tiled background
	std::unique_ptr<trmb::MapLayerNode> layer0(new trmb::MapLayerNode(mMap, 0));
	mSceneLayers[Background]->attachChild(std::move(layer0));

	// Add tiled houses
	std::unique_ptr<trmb::MapLayerNode> layer1(new trmb::MapLayerNode(mMap, 1));
	mSceneLayers[Background]->attachChild(std::move(layer1));

	// Add tiled roofs
	std::unique_ptr<trmb::MapLayerNode> layer2(new trmb::MapLayerNode(mMap, 2));
	mSceneLayers[Background]->attachChild(std::move(layer2));

	// ALW - Add darkess
	mSceneLayers[Sky]->attachChild(std::move(std::unique_ptr<Darkness>(new Darkness(mWindow))));

	// ALW - Add sprite and logic nodes
	std::vector<InteractiveObject>::const_iterator iter    = begin(mObjectGroups.getInteractiveGroup().getInteractiveObjects());
	std::vector<InteractiveObject>::const_iterator iterEnd = end(mObjectGroups.getInteractiveGroup().getInteractiveObjects());

	for (; iter != iterEnd; ++iter)
	{
		if (iter->getType() == "Barrel")
		{
			mSceneLayers[Update]->attachChild(std::move(std::unique_ptr<BarrelUpdateNode>(
				new BarrelUpdateNode(*iter, mTextures.get(Textures::ID::Tiles)))));

			mSceneLayers[Selection]->attachChild(std::move(std::unique_ptr<BarrelNode>(
				new BarrelNode(*iter, mWindow, mCamera.getView(), mUIBundle, mTextures, mSoundPlayer, mDaylightUI, mChatBoxUI))));
		}
		else if (iter->getType() == "Door")
		{
			mSceneLayers[Update]->attachChild(std::move(std::unique_ptr<DoorUpdateNode>(
				new DoorUpdateNode(*iter, mTextures.get(Textures::ID::Tiles)))));

			mSceneLayers[DoorSelection]->attachChild(std::move(std::unique_ptr<DoorNode>(
				new DoorNode(*iter, mWindow, mCamera.getView(), mUIBundle, mTextures, mSoundPlayer, mDaylightUI, mChatBoxUI))));
		}
		else if (iter->getType() == "Window")
		{
			mSceneLayers[Update]->attachChild(std::move(std::unique_ptr<WindowUpdateNode>(
				new WindowUpdateNode(*iter, mTextures.get(Textures::ID::Tiles)))));

			mSceneLayers[WindowSelection]->attachChild(std::move(std::unique_ptr<WindowNode>(
				new WindowNode(*iter, mWindow, mCamera.getView(), mUIBundle, mTextures, mSoundPlayer, mDaylightUI, mChatBoxUI))));
		}
		else if (iter->getType() == "Clinic")
		{
			mSceneLayers[Update]->attachChild(std::move(std::unique_ptr<ClinicUpdateNode>(
				new ClinicUpdateNode(*iter, mTextures.get(Textures::ID::Tiles)))));

			mSceneLayers[Selection]->attachChild(std::move(std::unique_ptr<ClinicNode>(
				new ClinicNode(*iter, mWindow, mCamera.getView(), mUIBundle, buildAttachedRects(*iter), mSoundPlayer, mDaylightUI
				, mChatBoxUI))));
		}
		else if (iter->getType() == "House")
		{
			mSceneLayers[Update]->attachChild(std::move(std::unique_ptr<HouseUpdateNode>(new HouseUpdateNode(*iter))));

			mSceneLayers[HouseSelection]->attachChild(std::move(std::unique_ptr<HouseNode>(
				new HouseNode(*iter, mWindow, mCamera.getView(), mUIBundle, buildAttachedRects(*iter), mFonts, mSoundPlayer))));
		}
		else
		{
			// ALW - TODO - Uncomment when all object types have been handled.
			//assert(("ALW - Logic Error: The interactive object type is not handled!", false));
		}
	}

	// ALW - Add mosquitoes
	const int mosquitoPopulation = 500;
	for (int i = 0; i < mosquitoPopulation; ++i)
	{
		mSceneLayers[Mosquitoes]->attachChild(std::move(std::unique_ptr<MosquitoNode>(new MosquitoNode(mTextures
			, getRandomSpawnPosition(), mWorldBounds, *mSceneLayers[HouseSelection]))));
	}

	// Add UIs
	mSceneLayers[UI]->attachChild(std::move(std::unique_ptr<BarrelUINode>(new BarrelUINode(mBarrelUI))));
	mSceneLayers[UI]->attachChild(std::move(std::unique_ptr<DoorUINode>(new DoorUINode(mDoorUI))));
	mSceneLayers[UI]->attachChild(std::move(std::unique_ptr<WindowUINode>(new WindowUINode(mWindowUI))));
	mSceneLayers[UI]->attachChild(std::move(std::unique_ptr<ClinicUINode>(new ClinicUINode(mClinicUI))));
	mSceneLayers[UI]->attachChild(std::move(std::unique_ptr<HouseUINode>(new HouseUINode(mHouseUI))));

	// Add player's character
	std::unique_ptr<HeroNode> player(new HeroNode(mWorldBounds, mCamera.getView()));
	mHero = player.get();
	mHero->setPosition(mHeroPosition);
	mSceneLayers[Camera]->attachChild(std::move(player));
}

std::vector<sf::FloatRect> World::buildAttachedRects(const InteractiveObject &interactiveObj)
{
	std::vector<InteractiveObject>::const_iterator iter    = begin(mObjectGroups.getInteractiveGroup().getInteractiveObjects());
	std::vector<InteractiveObject>::const_iterator iterEnd = end(mObjectGroups.getInteractiveGroup().getInteractiveObjects());
	std::vector<sf::FloatRect> attachedRects;

	// ALW - Store all the rects of objects (windows, doors, etc) with the house they are attached to. Later
	// ALW - the HouseNode object can use these rects to detect whether it is clicked or an attached object is.
	for (; iter != iterEnd; ++iter)
	{
		if (interactiveObj.getName() == iter->getAttachedTo())
			attachedRects.emplace_back(sf::FloatRect(iter->getX(), iter->getY(), iter->getWidth(), iter->getHeight()));
	}

	return attachedRects;
}

void World::generateSpawnPositions()
{
	// ALW - Width and height of map
	const int maxTilesWide = mObjectGroups.getInteractiveGroup().getWidth();
	const int maxTilesHigh = mObjectGroups.getInteractiveGroup().getHeight();

	const float tileWidth = 64;
	const float tileHeight = 64;

	for (int column = 0; column < maxTilesWide; ++column)
	{
		for (int row = 0; row < maxTilesHigh; ++row)
		{
			mSpawnPositions.emplace_back(sf::Vector2f(column * tileWidth, row * tileHeight));
		}
	}
}

sf::Vector2f World::getRandomSpawnPosition()
{
	return mSpawnPositions.at(mDistribution(mGenerator));
}
