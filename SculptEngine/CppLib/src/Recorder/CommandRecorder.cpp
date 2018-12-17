#include "CommandRecorder.h"

char const* const filenameRec = "c:\\data\\records\\1.rec";
char const* const filenamePlay = "c:\\data\\records\\1.play";

std::unique_ptr<CommandRecorder> CommandRecorder::_instance;	// Singleton

void CommandRecorder::Push(std::unique_ptr<Command> command)
{
#ifdef RECORD
	if(_outputFile == nullptr)
		_outputFile.reset(new std::ofstream(filenameRec, std::ifstream::binary));	// Lazy init
	command->Serialize(*_outputFile);
	_outputFile->flush();
#endif // RECORD
}

void CommandRecorder::Deserialize(Mesh& mesh)
{
	std::ifstream inputFile(filenamePlay, std::ofstream::binary);
	if(!inputFile)
	{
		fprintf(stderr, "Cannot open %s", filenamePlay);
		return;
	}
	while(inputFile.eof() == false)
	{
		COMMANDTYPE commandType;
		inputFile.get((char&) commandType);
		if(inputFile.eof())
			break;
		switch(commandType)
		{
		case COMMANDTYPE_START:
		{
			BRUSHTYPE brushType;
			inputFile.get((char&) brushType);
			_commandsBuffer.push_back(std::unique_ptr<Command>(new CommandStartStroke((BRUSHTYPE) brushType)));
			break;
		}
		case COMMANDTYPE_UPDATE:
		{
			BRUSHTYPE brushType;
			Vector3 rayOrigin;
			Vector3 rayDirection;
			float rayLength;
			float radius;
			float strengthRatio;
			inputFile.get((char&) brushType);
			inputFile.read((char*) &rayOrigin, sizeof(Vector3));
			inputFile.read((char*) &rayDirection, sizeof(Vector3));
			inputFile.read((char*) &rayLength, sizeof(float));
			inputFile.read((char*) &radius, sizeof(float));
			inputFile.read((char*) &strengthRatio, sizeof(float));
			_commandsBuffer.push_back(std::unique_ptr<Command>(new CommandUpdateStroke((BRUSHTYPE) brushType, Ray(rayOrigin, rayDirection, rayLength), radius, strengthRatio)));
			break;
		}
		case COMMANDTYPE_END:
		{
			BRUSHTYPE brushType;
			inputFile.get((char&) brushType);
			_commandsBuffer.push_back(std::unique_ptr<Command>(new CommandEndStroke((BRUSHTYPE) brushType)));
			break;
		}
		default:
			ASSERT(false);
			break;
		}
	}
	_brushDraw.reset(new BrushDraw(mesh));
	_brushInflate.reset(new BrushInflate(mesh));
	_brushFlatten.reset(new BrushFlatten(mesh));
	_brushDrag.reset(new BrushDrag(mesh));
	_brushDig.reset(new BrushDig(mesh));
	_brushSmear.reset(new BrushSmear(mesh));
	_brushSmooth.reset(new BrushSmooth(mesh));
	_brushCADDrag.reset(new BrushCADDrag(mesh));
}

bool CommandRecorder::PlayNextCommand()
{
	if(_curPlayyPos < _commandsBuffer.size())
	{
		Command* command = _commandsBuffer[_curPlayyPos++].get();
		switch(command->GetCommandType())
		{
			case COMMANDTYPE_START:
			{
				switch(dynamic_cast<CommandStartStroke*>(command)->GetBrushType())
				{
				case BRUSHTYPE_DRAW:
					_brushDraw->StartStroke();
					break;
				case BRUSHTYPE_INFLATE:
					_brushInflate->StartStroke();
					break;
				case BRUSHTYPE_FLATTEN:
					_brushFlatten->StartStroke();
					break;
				case BRUSHTYPE_DRAG:
					_brushDrag->StartStroke();
					break;
				case BRUSHTYPE_DIG:
					_brushDig->StartStroke();
					break;
				case BRUSHTYPE_SMEAR:
					_brushSmear->StartStroke();
					break;
				case BRUSHTYPE_SMOOTH:
					_brushSmooth->StartStroke();
					break;
				case BRUSHTYPE_CADDRAG:
					_brushCADDrag->StartStroke();
					break;
				}
				break;
			}
			case COMMANDTYPE_UPDATE:
			{
				CommandUpdateStroke* updateStroke = dynamic_cast<CommandUpdateStroke*>(command);
				switch(updateStroke->GetBrushType())
				{
				case BRUSHTYPE_DRAW:
					_brushDraw->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_INFLATE:
					_brushInflate->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_FLATTEN:
					_brushFlatten->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_DRAG:
					_brushDrag->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_DIG:
					_brushDig->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_SMEAR:
					_brushSmear->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_SMOOTH:
					_brushSmooth->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				case BRUSHTYPE_CADDRAG:
					_brushCADDrag->UpdateStroke(updateStroke->GetRay(), updateStroke->GetRadius(), updateStroke->GetStrengthRatio());
					break;
				}
				break;
			}
			case COMMANDTYPE_END:
			{
				switch(dynamic_cast<CommandEndStroke*>(command)->GetBrushType())
				{
				case BRUSHTYPE_DRAW:
					_brushDraw->EndStroke();
					break;
				case BRUSHTYPE_INFLATE:
					_brushInflate->EndStroke();
					break;
				case BRUSHTYPE_FLATTEN:
					_brushFlatten->EndStroke();
					break;
				case BRUSHTYPE_DRAG:
					_brushDrag->EndStroke();
					break;
				case BRUSHTYPE_DIG:
					_brushDig->EndStroke();
					break;
				case BRUSHTYPE_SMEAR:
					_brushSmear->EndStroke();
					break;
				case BRUSHTYPE_SMOOTH:
					_brushSmooth->EndStroke();
					break;
				case BRUSHTYPE_CADDRAG:
					_brushCADDrag->EndStroke();
					break;
				}
				break;
			}
			default:
				ASSERT(false);
				break;
		}
		return true;
	}
	return false;
}

void CommandStartStroke::Serialize(std::ofstream &outStream)
{
	Command::Serialize(outStream);
	outStream.put(_brushType);
}

void CommandUpdateStroke::Serialize(std::ofstream &outStream)
{
	Command::Serialize(outStream);
	outStream.put(_brushType);
	outStream.write((const char*) &(_ray.GetOrigin()), sizeof(Vector3));
	outStream.write((const char*) &(_ray.GetDirection()), sizeof(Vector3));
	float length = _ray.GetLength();
	outStream.write((const char*) &length, sizeof(float));
	outStream.write((const char*) &_radius, sizeof(float));
	outStream.write((const char*) &_strengthRatio, sizeof(float));
}

void CommandEndStroke::Serialize(std::ofstream &outStream)
{
	Command::Serialize(outStream);
	outStream.put(_brushType);
}

void Command::Serialize(std::ofstream &outStream)
{
	outStream.put(_commandType);
}
