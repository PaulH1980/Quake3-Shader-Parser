#include <Events/DefaultEvents.h>
#include <Misc/Q3Entities.h>

namespace Misc
{
	Q3Entity::Q3Entity(ContextPointer engine) : EngineObject(engine)
	{
		subscribeToEvent( CREATE_EVENT_CALLBACK(Q3Entity, beginFrame), EVENT_TYPE(TIMER_BEGIN_FRAME));
	}

	

	void Q3Entity::beginFrame(App::Event& evt)
	{
		auto timeStep = evt.getValue<float>(TIMER_BEGIN_FRAME::PARAM_TIME_STEP);
		update(timeStep);
		m_totalTime += timeStep;
	}

	void Q3Entity::update(float timeStep)
	{
		m_objectTime += timeStep;
	}

}


