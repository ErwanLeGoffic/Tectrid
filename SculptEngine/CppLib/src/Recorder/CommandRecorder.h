#ifndef _COMMANDRECORDER_H_
#define _COMMANDRECORDER_H_

#include <vector>
#include <memory>
#include <fstream>
#include "Brushes\BrushDraw.h"
#include "Brushes\BrushInflate.h"
#include "Brushes\BrushFlatten.h"
#include "Brushes\BrushDrag.h"
#include "Brushes\BrushDig.h"
#include "Brushes\BrushSmear.h"
#include "Brushes\BrushSmooth.h"
#include "Brushes\BrushCADDrag.h"

//#define RECORD

enum COMMANDTYPE : unsigned char
{
	COMMANDTYPE_START,
	COMMANDTYPE_UPDATE,
	COMMANDTYPE_END
};

class Command
{
public:
	virtual void Serialize(std::ofstream &outStream);
	virtual void Deserialize() = 0;
	COMMANDTYPE GetCommandType() const { return _commandType; }

protected:
	Command(COMMANDTYPE commandType) : _commandType(commandType) {}
	COMMANDTYPE const _commandType;
};

class CommandStartStroke : public Command
{
public:
	CommandStartStroke(BRUSHTYPE brushType) : Command(COMMANDTYPE_START), _brushType(brushType) {}
	virtual void Serialize(std::ofstream &outStream);
	virtual void Deserialize() {}
	BRUSHTYPE GetBrushType() const { return _brushType; }

private:
	BRUSHTYPE _brushType;
};

class CommandUpdateStroke : public Command
{
public:
	CommandUpdateStroke(BRUSHTYPE brushType, Ray const& ray, float effectDist, float strengthRatio) : Command(COMMANDTYPE_UPDATE), _brushType(brushType), _ray(ray), _radius(effectDist), _strengthRatio(strengthRatio) {}
	virtual void Serialize(std::ofstream &outStream);
	virtual void Deserialize() {}
	BRUSHTYPE GetBrushType() const { return _brushType; }
	Ray const& GetRay() const { return _ray; }
	float GetRadius() const { return _radius; }
	float GetStrengthRatio() const { return _strengthRatio; }

private:
	BRUSHTYPE _brushType;
	Ray _ray;
	float _radius;
	float _strengthRatio;
};

class CommandEndStroke : public Command
{
public:
	CommandEndStroke(BRUSHTYPE brushType) : Command(COMMANDTYPE_END), _brushType(brushType) {}
	virtual void Serialize(std::ofstream &outStream);
	virtual void Deserialize() {}
	BRUSHTYPE GetBrushType() const { return _brushType; }

private:
	BRUSHTYPE _brushType;
};

class CommandRecorder
{
public:	
	static CommandRecorder& GetInstance()	// Singleton
	{
		if(_instance == nullptr)
			_instance.reset(new CommandRecorder());
		return *(_instance.get());
	}

	void Push(std::unique_ptr<Command> command);
	void Deserialize(Mesh& mesh);
	bool PlayNextCommand();

#ifdef BRUSHES_DEBUG_DRAW
	BrushDrag const* GetDragBrush() const { return _brushDrag.get(); }
#endif // BRUSHES_DEBUG_DRAW
	
private:
	CommandRecorder(): _curPlayyPos(0) {}

	std::unique_ptr<std::ofstream> _outputFile;
	unsigned int _curPlayyPos;
	std::vector<std::unique_ptr<Command> > _commandsBuffer;
	std::unique_ptr<BrushDraw> _brushDraw;
	std::unique_ptr<BrushInflate> _brushInflate;
	std::unique_ptr<BrushFlatten> _brushFlatten;
	std::unique_ptr<BrushDrag> _brushDrag;
	std::unique_ptr<BrushDig> _brushDig;
	std::unique_ptr<BrushSmear> _brushSmear;
	std::unique_ptr<BrushSmooth> _brushSmooth;
	std::unique_ptr<BrushCADDrag> _brushCADDrag;

	static std::unique_ptr<CommandRecorder> _instance;	// Singleton
};

#endif // _COMMANDRECORDER_H_