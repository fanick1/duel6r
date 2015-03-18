/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include "World.h"
#include "Util.h"
#include "ElevatorList.h"
#include "Format.h"
#include "Lang.h"
#include "DataException.h"
#include "json/Json.h"
#include "Defines.h"

namespace Duel6
{
	World::World(Float32 animationSpeed, Float32 waveHeight, Console& console)
		: console(console), animationSpeed(animationSpeed), waveHeight(waveHeight)
	{}

	void World::initialize(const std::string& blockMetaFile)
	{
		loadBlockMeta(blockMetaFile);
	}

	Block::Type World::determineBlockType(const std::string& kind) const
	{
		static std::vector<std::string> typeNames = {
			"EMPTY_SPACE",
			"WALL",
			"WATER",
			"FRONT_SPRITE",
			"BACK_SPRITE",
			"FRONT_AND_BACK_SPRITE",
			"FRONT4_SPRITE",
			"BACK4_SPRITE",
			"WATERFALL"
		};

		auto typeIter = std::find(typeNames.begin(), typeNames.end(), kind);
		if (typeIter == typeNames.end())
		{
			D6_THROW(DataException, std::string("Unknown block type: ") + kind);
		}

		return (Block::Type)(typeIter - typeNames.begin());
	}

	void World::loadBlockMeta(const std::string& path)
	{
		console.printLine(Format("Loading block meta data from: {0}") << path);
		blockMeta.clear();

		Json::Parser parser;
		const Json::Value& root = parser.parse(path);

		for (Size i = 0; i < root.getLength(); i++)
		{
			const Json::Value& block = root.get(i);
			Block::Type type = determineBlockType(block.get("kind").asString());
			const Json::Value& animations = block.get("animations");
			std::vector<Int32> textures;
			for (Size j = 0; j < animations.getLength(); j++)
			{
				textures.push_back(animations.get(j).asInt());
			}
			blockMeta.push_back(Block(blockMeta.size(), type, std::move(textures)));
		}
	}

	void World::loadLevel(const std::string& path, Size background, bool mirror)
	{
		console.printLine(Format("Loading level: {0}, mirror: {1}") << path << mirror);
		this->background = background;
		levelData.clear();
		waterLevel = 0;
		Json::Parser parser;
		const Json::Value& root = parser.parse(path);

		width = root.get("width").asInt();
		height = root.get("height").asInt();
		
		Int32 blockCount = width * height;
		const Json::Value& blocks = root.get("blocks");
		levelData.resize(blockCount);
		for (Size i = 0; i < blocks.getLength(); i++)
		{
			levelData[i] = blocks.get(i).asInt();
		}

		console.printLine("Loading elevators");
		ELEV_Clear();
		Size elevators = root.get("elevators").getLength();
		for (Size i = 0; i < elevators; i++)
		{
			Elevator elevator;
			const Json::Value& points = root.get("elevators").get(i).get("controlPoints");
			for (Size j = 0; j < points.getLength(); j++)
			{
				Int32 x = points.get(j).get("x").asInt();
				Int32 y = points.get(j).get("y").asInt();
				elevator.addControlPoint(Elevator::ControlPoint(mirror ? width - 1 - x : x, height - y));
			}
			ELEV_Add(elevator);
		}

		if (mirror)
		{
			mirrorLevelData();
		}
	}

	void World::mirrorLevelData()
	{
		for (Int32 y = 0; y < height; y++)
		{
			for (Int32 x = 0; x < width / 2; x++)
			{
				std::swap(levelData[y * width + x], levelData[y * width + width - 1 - x]);
			}
		}
	}

	void World::prepareFaces()
	{
		console.printLine("Preparing faces");
		animWait = 0;
		addWallFaces();
		addSpriteFaces();
		addWaterFaces();

		floatingVertexes.build(water, waveHeight, console);
	}

	void World::update(Float32 elapsedTime)
	{
		animWait += elapsedTime;
		if (animWait > animationSpeed)
		{
			animWait = 0;
			walls.nextFrame();
			sprites.nextFrame();
			water.nextFrame();
		}

		floatingVertexes.update(elapsedTime);
	}

	void World::raiseWater()
	{
        if(waterLevel < getSizeY() - 2)
        {
			waterLevel++;
			for(Int32 x = 1; x < getSizeX() - 1; x++)
			{
				if(!isWall(x, waterLevel, false))
				{
					setBlock(4, x, waterLevel);
				}
			}
			prepareFaces();
        }
	}

	void World::addWallFaces()
	{
		walls.clear();

		for (Int32 y = 0; y < getSizeY(); y++)
		{
			for (Int32 x = 0; x < getSizeX(); x++)
			{
				const Block& block = getBlockMeta(x, y);

				if (block.is(Block::Wall))
				{
					addWall(block, x, y);
				}
			}
		}

		walls.optimize();
	}

	void World::addSpriteFaces()
	{
		sprites.clear();

		for (Int32 y = 0; y < getSizeY(); y++)
		{
			for (Int32 x = 0; x < getSizeX(); x++)
			{
				const Block& block = getBlockMeta(x, y);

				if (block.is(Block::FrontAndBackSprite))
				{
					addSprite(sprites, block, x, y, 1.0f);
					addSprite(sprites, block, x, y, 0.0f);
				}
				else if (block.is(Block::FrontSprite))
				{
					addSprite(sprites, block, x, y, 1.0f);
				}
				else if (block.is(Block::BackSprite))
				{
					addSprite(sprites, block, x, y, 0.0f);
				}
				else if (block.is(Block::Front4Sprite))
				{
					addSprite(sprites, block, x, y, 0.75f);
				}
				else if (block.is(Block::Back4Sprite))
				{
					addSprite(sprites, block, x, y, 0.25f);
				}
			}
		}

		sprites.optimize();
	}

	void World::addWaterFaces()
	{
		water.clear();

		for (Int32 y = 0; y < getSizeY(); y++)
		{
			for (Int32 x = 0; x < getSizeX(); x++)
			{
				const Block& block = getBlockMeta(x, y);

				if (block.is(Block::Waterfall))
				{
					addSprite(water, block, x, y, 0.75);
				}
				else if (block.is(Block::Water))
				{
					addWater(block, x, y);
				}
			}
		}

		water.optimize();
	}

	void World::addWall(const Block& block, Int32 x, Int32 y)
	{
		walls.addFace(Face(block))
			.addVertex(Vertex(0, x, y + 1, 1))
			.addVertex(Vertex(1, x + 1, y + 1, 1))
			.addVertex(Vertex(2, x + 1, y, 1))
			.addVertex(Vertex(3, x, y, 1));


#ifdef D6_RENDER_BACKS
		faceList.addFace(Face(block))
			.addVertex(Vertex(0, x + 1, y + 1, 0))
			.addVertex(Vertex(1, x, y + 1, 0))
			.addVertex(Vertex(2, x, y, 0))
			.addVertex(Vertex(3, x + 1, y, 0));
#endif

		if (!isWall(x - 1, y, false))
		{
			walls.addFace(Face(block))
				.addVertex(Vertex(0, x, y + 1, 0))
				.addVertex(Vertex(1, x, y + 1, 1))
				.addVertex(Vertex(2, x, y, 1))
				.addVertex(Vertex(3, x, y, 0));
		}
		if (!isWall(x + 1, y, false))
		{
			walls.addFace(Face(block))
				.addVertex(Vertex(0, x + 1, y + 1, 1))
				.addVertex(Vertex(1, x + 1, y + 1, 0))
				.addVertex(Vertex(2, x + 1, y, 0))
				.addVertex(Vertex(3, x + 1, y, 1));
		}
		if (!isWall(x, y + 1, false))
		{
			walls.addFace(Face(block))
				.addVertex(Vertex(0, x, y + 1, 1))
				.addVertex(Vertex(1, x, y + 1, 0))
				.addVertex(Vertex(2, x + 1, y + 1, 0))
				.addVertex(Vertex(3, x + 1, y + 1, 1));
		}
		if (!isWall(x, y - 1, false))
		{
			walls.addFace(Face(block))
				.addVertex(Vertex(0, x, y, 1))
				.addVertex(Vertex(1, x + 1, y, 1))
				.addVertex(Vertex(2, x + 1, y, 0))
				.addVertex(Vertex(3, x, y, 0));
		}
	}

	void World::addWater(const Block& block, Int32 x, Int32 y)
	{
		bool topWater = !isWater(x, y + 1);
		Vertex::Flag flowFlag = topWater ? Vertex::Flag::Flow : Vertex::Flag::None;

		water.addFace(Face(block))
			.addVertex(Vertex(0, x, y + 1, 1, flowFlag))
			.addVertex(Vertex(1, x + 1, y + 1, 1, flowFlag))
			.addVertex(Vertex(2, x + 1, y, 1))
			.addVertex(Vertex(3, x, y, 1));

		water.addFace(Face(block))
			.addVertex(Vertex(0, x + 1, y + 1, 0, flowFlag))
			.addVertex(Vertex(1, x, y + 1, 0, flowFlag))
			.addVertex(Vertex(2, x, y, 0))
			.addVertex(Vertex(3, x + 1, y, 0));

		if (topWater)
		{
			water.addFace(Face(block))
				.addVertex(Vertex(0, x, y + 1, 1, Vertex::Flag::Flow))
				.addVertex(Vertex(1, x, y + 1, 0, Vertex::Flag::Flow))
				.addVertex(Vertex(2, x + 1, y + 1, 0, Vertex::Flag::Flow))
				.addVertex(Vertex(3, x + 1, y + 1, 1, Vertex::Flag::Flow));
		}
	}

	void World::addSprite(FaceList& faceList, const Block& block, Int32 x, Int32 y, Float32 z)
	{
		Float32 fx = Float32(x), fy = Float32(y);
		bool bottomWaterfall = (block.is(Block::Waterfall) && isWater(x, y - 1));
		Vertex::Flag flowFlag = bottomWaterfall ? Vertex::Flag::Flow : Vertex::Flag::None;

		faceList.addFace(Face(block))
			.addVertex(Vertex(0, fx, fy + 1, z))
			.addVertex(Vertex(1, fx + 1, fy + 1, z))
			.addVertex(Vertex(2, fx + 1, fy, z, flowFlag))
			.addVertex(Vertex(3, fx, fy, z, flowFlag));
	}

	Water::Type World::getWaterType(Int32 x, Int32 y) const
	{
		if (isWater(x, y))
		{
			Uint16 block = getBlock(x, y);
			if (block == 4)
			{
				return Water::Type::Blue;
			}
			else if (block == 16)
			{
				return Water::Type::Red;
			}
			else if (block == 33)
			{
				return Water::Type::Green;
			}
		}

		return Water::Type::None;
	}
}