#include "TickerScheduler.h"

void tickerFlagHandle(volatile bool * flag)
{
    if (!*flag)
        *flag = true;
}

TickerScheduler::TickerScheduler(uint8_t size)
{
    this->items = new TickerSchedulerItem[size];
    this->size = size;
}

TickerScheduler::~TickerScheduler()
{
    for (uint8_t i = 0; i < this->size; i++)
    {
        this->remove(i);
        yield();
    }

    delete[] this->items;
    this->items = NULL;
    this->size = 0;
}

void TickerScheduler::handleTicker(tscallback_t f, volatile bool * flag)
{
    if (*flag)
    {
        yield();
        *flag = false;
        yield();
        f();
        yield();
    }
}

bool TickerScheduler::add(uint8_t i, uint32_t period, tscallback_t f, boolean shouldFireNow)
{
    if (i >= this->size || this->items[i].is_used)
        return false;

    this->items[i].cb = f;
    this->items[i].flag = shouldFireNow;
    this->items[i].period = period;
    this->items[i].is_used = true;

    return enable(i);
}

bool TickerScheduler::remove(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].is_used = false;
    this->items[i].t.detach();
    this->items[i].flag = false;
    this->items[i].cb = NULL;

    return true;
}

bool TickerScheduler::changePeriod(uint8_t i, uint32_t period)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].period = period;

    return true;
}

bool TickerScheduler::disable(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].t.detach();

    return true;
}

bool TickerScheduler::enable(uint8_t i)
{
    if (i >= this->size || !this->items[i].is_used)
        return false;

    this->items[i].t.attach_ms(this->items[i].period, tickerFlagHandle, &this->items[i].flag);

    return true;
}

void TickerScheduler::disableAll()
{
    for (uint8_t i = 0; i < this->size; i++)
        disable(i);
}

void TickerScheduler::enableAll()
{
    for (uint8_t i = 0; i < this->size; i++)
        enable(i);
}

void TickerScheduler::update()
{
    for (uint8_t i = 0; i < this->size; i++)
    {
		if (this->items[i].is_used)
		{
			#ifdef ARDUINO_ARCH_AVR
			this->items[i].t.Tick();
			#endif

			handleTicker(this->items[i].cb, &this->items[i].flag);
		}
        yield();
    }
}
