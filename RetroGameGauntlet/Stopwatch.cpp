/*
    Copyright (C) 2014 by Elvis Angelaccio <elvis.angelaccio@kdemail.net>

    This file is part of Kronometer.

    Kronometer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Kronometer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kronometer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stopwatch.h"

#include <QTimerEvent>
#include <QCoreApplication>
#include <QJsonObject>

Stopwatch::Stopwatch(QObject *parent) :
    QObject(parent),
    timerId(INACTIVE_TIMER_ID),
    state(State::INACTIVE),
    granularity(HUNDREDTHS),
    zero(0, 0)
{}

void Stopwatch::setGranularity(Granularity g)
{
    granularity = g;
}

bool Stopwatch::isRunning() const
{
    return state == State::RUNNING;
}

bool Stopwatch::isPaused() const
{
    return state == State::PAUSED;
}

bool Stopwatch::isInactive() const
{
    return state == State::INACTIVE;
}

qint64 Stopwatch::raw() const
{
    return accumulator;
}

void Stopwatch::setRaw(qint64 _ms)
{
    accumulator = _ms;
}

bool Stopwatch::initialize(qint64 rawData)
{
    if (state != State::INACTIVE or rawData <= 0) {
        return false;
    }

    accumulator = rawData;
    state = State::PAUSED;
    emit time(zero.addMSecs(accumulator));  // it signals that has been deserialized and can be resumed

    return true;
}

void Stopwatch::onStart()
{
    if (state == State::INACTIVE) {
        accumulator = 0;
        elapsedTimer.start();

        if (timerId == INACTIVE_TIMER_ID) {
            timerId = startTimer(granularity);
        }
    }
    else if (state == State::PAUSED) {
        elapsedTimer.restart();
        timerId = startTimer(granularity);
    }

    state = State::RUNNING;
}

void Stopwatch::onPause()
{
    if (elapsedTimer.isValid()) {
        accumulator += elapsedTimer.elapsed();
    }

    elapsedTimer.invalidate();
    state = State::PAUSED;
}

void Stopwatch::onReset()
{
    elapsedTimer.invalidate();          // if state is running, it will emit a zero time at next timerEvent() call
    QCoreApplication::processEvents();
    emit time(zero);
    state = State::INACTIVE;
}

void Stopwatch::onLap()
{
    qint64 lapTime = 0;

    lapTime += accumulator;

    if (elapsedTimer.isValid()) {
        lapTime += elapsedTimer.elapsed();
    }

    emit lap(zero.addMSecs(lapTime));
}

void Stopwatch::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timerId) {      // forward undesired events
        QObject::timerEvent(event);
        return;
    }

    qint64 t = 0;

    t += accumulator;

    if (elapsedTimer.isValid()) {
        t += elapsedTimer.elapsed();
        emit time(zero.addMSecs(t));
    }
    else {
        killTimer(timerId);
        timerId = INACTIVE_TIMER_ID;
    }
}
