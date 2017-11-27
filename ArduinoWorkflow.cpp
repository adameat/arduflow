#include "ArduinoWorkflow.h"
#include <avr/wdt.h>
//#include <LowPower.h>

namespace AW {

void TActorContext::Send(TActor* sender, TActor* recipient, TEventPtr event) const {
	ActorLib.Send(sender, recipient, event);
}

void TActorContext::SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) const {
	ActorLib.SendImmediate(sender, recipient, event);
}

void TActorContext::Resend(TActor* recipient, TEventPtr event) const {
	ActorLib.Resend(recipient, event);
}

void TActorContext::ResendImmediate(TActor* recipient, TEventPtr event) const {
	ActorLib.ResendImmediate(recipient, event);
}

TActorLib::TActorLib() {
	wdt_disable();
}

void TActorLib::Register(TActor* actor) {
	TActor* itActor = Actors;
	if (itActor == nullptr) {
		Actors = actor;
	} else {
		while (itActor->NextActor != nullptr) {
			itActor = itActor->NextActor;
		}
		itActor->NextActor = actor;
	}
	TEventPtr bootstrapEvent = new TEventBootstrap;
	Send(actor, actor, bootstrapEvent);
	wdt_enable(WDTO_8S);
}

void TActorLib::Run() {
	TActorContext context(*this);
	TActor* itActor = Actors;
	TTime minSleep = TTime::Max();
	while (itActor != nullptr) {
		wdt_reset();
		auto& events(itActor->Events);
		auto end(events.end());
		auto itEvent = events.begin();
		while (itEvent != events.end() && itEvent != end) {
			TEventPtr event = events.pop_value(itEvent);
			if (context.Now < event->NotBefore) {
				TTime sleep = event->NotBefore - context.Now;
				if (minSleep > sleep) {
					minSleep = sleep;
				}
				auto itEnd = events.push_back(event);
				if (end == events.end()) {
					end = itEnd;
				}
			} else {
				minSleep = TTime::Zero();
				itActor->OnEvent(event, context);
				if (itEvent != events.begin())
					break;
			}
		}
		itActor = itActor->NextActor;
	}
	//if (true) {
	//	LowPower.powerStandby(SLEEP_30MS, ADC_ON, BOD_ON);
	//	wdt_enable(WDTO_8S);
	//}
}

void TActorLib::Send(TActor* sender, TActor* recipient, TEventPtr event) {
	event->Sender = sender;
	//event->Recipient = recipient;
	recipient->Events.push_back(event);
}

void TActorLib::SendImmediate(TActor* sender, TActor* recipient, TEventPtr event) {
	event->Sender = sender;
	//event->Recipient = recipient;
	recipient->Events.push_front(event);
}

void TActorLib::Resend(TActor* recipient, TEventPtr event) {
	recipient->Events.push_back(event);
}

void TActorLib::ResendImmediate(TActor* recipient, TEventPtr event) {
	recipient->Events.push_front(event);
}

String TTime::AsString() const {
	return String(Value);
}

}