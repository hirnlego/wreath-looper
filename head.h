/**
 * Inspired by Monome softcut's subhead class:
 * https://github.com/monome/softcut-lib/blob/main/softcut-lib/src/SubHead.cpp
 */

#pragma once

#include <stdint.h>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace wreath
{
    constexpr int32_t kMinLoopLengthSamples{48};
    constexpr int32_t kMinLoopLengthForFade{4800};
    constexpr int32_t kSamplesToFade{1200};

    enum Fade
    {
        NO_FADE,
        SMOOTH,
        IN,
        OUT,
    };

    enum Type
    {
        READ,
        WRITE,
    };

    enum Action
    {
        NO_ACTION,
        LOOP,
        INVERT,
        STOP,
    };

    enum Movement
    {
        NORMAL,
        PENDULUM,
        DRUNK,
    };

    enum Direction
    {
        BACKWARDS = -1,
        FORWARD = 1
    };

    class Head
    {
    private:
        const Type type_;
        float *buffer_;

        int32_t maxBufferSamples_{}; // The whole buffer length in samples
        int32_t bufferSamples_{};    // The written buffer length in samples

        int32_t intIndex_{};
        float index_{};
        float rate_{};
        float fadeIndex_{};

        int32_t loopStart_{};
        int32_t loopEnd_{};
        int32_t loopLength_{};

        bool run_{};
        bool looping_{};

        Movement movement_{};
        Direction direction_{};
        Fade mustFade_{};

        float snapshotValue_{};
        float fastAvg{};
        float slowAvg{};
        float prevDifference{};
        float currentValue_{};
        float inputValue_{};
        bool isStopping_{};

        Action HandleLoopAction()
        {
            // Handle normal loop boundaries.
            if (loopEnd_ > loopStart_)
            {
                // Forward direction.
                if (Direction::FORWARD == direction_)
                {
                    if (looping_ && intIndex_ > loopEnd_)
                    {
                        if (Movement::PENDULUM == movement_ && looping_)
                        {
                            SetIndex(loopEnd_ - (index_ - loopEnd_));

                            return Action::INVERT;
                        }
                        else
                        {
                            SetIndex((loopStart_ + (index_ - loopEnd_)) - 1);

                            return Action::LOOP;
                        }
                    }
                    // When the head is not looping, and while it's not already
                    // stopping, stop it and allow for a fade out.
                    else if (!isStopping_ && !looping_ && intIndex_ > loopEnd_ - SamplesToFade())
                    {
                        return Action::STOP;
                    }
                }
                // Backwards direction.
                else {
                    if (looping_ && intIndex_ < loopStart_)
                    {
                        if (Movement::PENDULUM == movement_ && looping_)
                        {
                            SetIndex(loopStart_ + (loopStart_ - index_));

                            return Action::INVERT;
                        }
                        else
                        {
                            SetIndex((loopEnd_ - std::abs(loopStart_ - index_)) + 1);

                            return Action::LOOP;
                        }
                    }
                    // When the head is not looping, and while it's not already
                    // stopping, stop it and allow for a fade out.
                    else if (!isStopping_ && !looping_ && intIndex_ < loopStart_ + SamplesToFade())
                    {
                        return Action::STOP;
                    }
                }
            }
            // Handle inverted loop boundaries (end point comes before start point).
            else
            {
                float frame = bufferSamples_ - 1;
                if (Direction::FORWARD == direction_)
                {
                    if (intIndex_ > frame)
                    {
                        // Wrap-around.
                        SetIndex((index_ - frame) - 1);

                        return looping_ ? Action::LOOP : Action::STOP;
                    }
                    else if (intIndex_ > loopEnd_ && intIndex_ < loopStart_)
                    {
                        if (Movement::PENDULUM == movement_ && looping_)
                        {
                            // Max to avoid overflow.
                            SetIndex(std::max(loopEnd_ - (index_ - loopEnd_), 0.f));

                            return Action::INVERT;
                        }
                        else
                        {
                            // Min to avoid overflow.
                            SetIndex(std::min(loopStart_ + (index_ - loopEnd_) - 1, frame));

                            return looping_ ? Action::LOOP : Action::STOP;
                        }
                    }
                }
                else
                {
                    if (intIndex_ < 0)
                    {
                        // Wrap-around.
                        SetIndex((frame - std::abs(index_)) + 1);

                        return looping_ ? Action::LOOP : Action::STOP;
                    }
                    else if (intIndex_ > loopEnd_ && intIndex_ < loopStart_)
                    {
                        if (Movement::PENDULUM == movement_ && looping_)
                        {
                            // Min to avoid overflow.
                            SetIndex(std::min(loopStart_ + (loopStart_ - index_), frame));

                            return Action::INVERT;
                        }
                        else
                        {
                            // Max to avoid overflow.
                            SetIndex(std::max(loopEnd_ - (loopStart_ - index_) + 1, 0.f));

                            return looping_ ? Action::LOOP : Action::STOP;
                        }
                    }
                }
            }

            return Action::NO_ACTION;
        }

        int32_t WrapIndex(int32_t index)
        {
            // Handle normal loop boundaries.
            if (loopEnd_ > loopStart_)
            {
                // Forward direction.
                if (index > loopEnd_)
                {
                    if (Movement::PENDULUM == movement_)
                    {
                        index = loopEnd_ - (index - loopEnd_);
                    }
                    else
                    {
                        index = (FORWARD == direction_) ? (loopStart_ + (index - loopEnd_)) - 1 : 0;
                    }
                }
                // Backwards direction.
                else if (index < loopStart_)
                {
                    if (Movement::PENDULUM == movement_)
                    {
                        index = loopStart_ + (loopStart_ - index);
                    }
                    else
                    {
                        index = (BACKWARDS == direction_) ? (loopEnd_ - std::abs(loopStart_ - index)) + 1 : 0;
                    }
                }
            }
            // Handle inverted loop boundaries (end point comes before start point).
            else
            {
                int32_t frame{bufferSamples_ - 1};
                if (index > frame)
                {
                    index = (index - frame) - 1;
                }
                else if (index < 0)
                {
                    // Wrap-around.
                    index = (frame - std::abs(index)) + 1;
                }
                else if (index > loopEnd_ && index < loopStart_)
                {
                    if (FORWARD == direction_)
                    {
                        // Max/min to avoid overflow.
                        index = (Movement::PENDULUM == movement_) ? std::max(loopEnd_ - (index - loopEnd_), static_cast<int32_t>(0)) : std::min(loopStart_ + (index - loopEnd_) - 1, frame);
                    }
                    else
                    {
                        // Max/min to avoid overflow.
                        index = (Movement::PENDULUM == movement_) ? std::min(loopStart_ + (loopStart_ - index), frame) : std::max(loopEnd_ - (loopStart_ - index) + 1, static_cast<int32_t>(0));
                    }
                }
            }

            return index;
        }

        void CalculateLoopEnd()
        {
            if (loopStart_ + loopLength_ > bufferSamples_)
            {
                loopEnd_ = (loopStart_ + loopLength_) - bufferSamples_ - 1;
            }
            else
            {
                loopEnd_ = loopStart_ + loopLength_ - 1;
            }
        }

    public:
        Head(Type type) : type_{type} {}
        ~Head() {}

        void Reset()
        {
            intIndex_ = 0;
            index_ = 0.f;
            rate_ = 1.f;
            loopStart_ = 0;
            loopEnd_ = 0;
            loopLength_ = 0;
            run_ = true;
            looping_ = true;
            movement_ = NORMAL;
            direction_ = FORWARD;
        }

        void Init(float *buffer, int32_t maxBufferSamples)
        {
            buffer_ = buffer;
            maxBufferSamples_ = maxBufferSamples;
            Reset();
        }

        int32_t SetLoopStart(int32_t start)
        {
            loopStart_ = std::min(std::max(start, static_cast<int32_t>(0)), bufferSamples_ - 1);
            CalculateLoopEnd();
            if (!looping_)
            {
                ResetPosition();
            }

            return loopStart_;
        }

        int32_t SetLoopLength(int32_t length)
        {
            loopLength_ = std::min(std::max(length, static_cast<int32_t>(kMinLoopLengthSamples)), bufferSamples_);
            CalculateLoopEnd();

            return loopLength_;
        }

        int32_t SamplesToFade()
        {
            return std::min(kSamplesToFade, loopLength_);
        }

        inline void SetRate(float rate) { rate_ = rate; }
        inline void SetMovement(Movement movement) { movement_ = movement; }
        inline void SetDirection(Direction direction) { direction_ = direction; }
        inline void SetIndex(float index)
        {
            index_ = index;
            intIndex_ = static_cast<int32_t>(std::floor(index));
        }
        inline void ResetPosition()
        {
            SetIndex(FORWARD == direction_ ? loopStart_ : loopEnd_);
        }
        float UpdatePosition()
        {
            if (!run_)
            {
                return index_;
            }

            float index = index_ + (rate_ * direction_);
            SetIndex(index);
            Action action = HandleLoopAction();

            if (READ == type_)
            {
                switch (action)
                {
                case STOP:
                    Stop(true);
                    break;

                case INVERT:
                    ToggleDirection();
                    break;

                case LOOP:
                    break;

                default:
                    break;
                }
            }

            return index_;
        }

        bool IsFading()
        {
            return Fade::NO_FADE != mustFade_;
        }

        void SetUpFade(Fade fade)
        {
            snapshotValue_ = ReadAt(index_);
            mustFade_ = fade;
            fadeIndex_ = 0;
        }

        float Read()
        {
            currentValue_ = ReadAt(index_);
            int32_t samplesToFade = SamplesToFade();

            // Gradually start reading, fading from zero to the buffered value.
            if (Fade::IN == mustFade_)
            {
                // Actually start reading.
                Run(false);
                float pos = fadeIndex_ * (1.f / samplesToFade);
                if (fadeIndex_ < samplesToFade - 1)
                {
                    currentValue_ *= pos;
                    fadeIndex_ += rate_;
                }
                else
                {
                    mustFade_ = Fade::NO_FADE;
                }
            }
            // Gradually stop reading, fading from the buffered value to zero.
            else if (Fade::OUT == mustFade_)
            {
                float pos = fadeIndex_ * (1.f / samplesToFade);
                if (fadeIndex_ < samplesToFade - 1)
                {
                    currentValue_ *= 1.0f - pos;
                    fadeIndex_ += rate_;
                }
                else
                {
                    mustFade_ = Fade::NO_FADE;
                    // Actually stop reading.
                    Stop(false);
                }
            }

            // Apply switch-and-ramp technique to smooth the read value.
            // http://msp.ucsd.edu/techniques/v0.11/book-html/node63.html
            if (Fade::SMOOTH == mustFade_)
            {
                float pos = fadeIndex_ * (1.f / samplesToFade);
                if (fadeIndex_ < samplesToFade - 1)
                {
                    float delta = snapshotValue_ - currentValue_;
                    currentValue_ += delta * (1.0f - pos);
                    fadeIndex_ += rate_;
                }
                else
                {
                    mustFade_ = Fade::NO_FADE;
                }
            }

            return run_ ? currentValue_ : 0;
        }

        float ReadAt(float index)
        {
            int32_t intPos = static_cast<int32_t>(std::floor(index));
            float value = buffer_[intPos];
            float frac = index - intPos;

            // Interpolate value only it the index has a fractional part.
            if (frac > std::numeric_limits<float>::epsilon())
            {
                float value2 = buffer_[WrapIndex(intPos + direction_)];

                return value + (value2 - value) * frac;
            }

            return value;
        }

        void Write(float value)
        {
            currentValue_ = ReadAt(index_);
            int32_t samplesToFade = SamplesToFade();

            // Gradually start writing, fading from the buffered value to the input
            // signal.
            if (Fade::IN == mustFade_)
            {
                // Actually start writing.
                Run(false);
                float pos = fadeIndex_ * (1.f / samplesToFade);
                if (fadeIndex_ < samplesToFade - 1)
                {
                    value = currentValue_ * (1.0f - pos) + (value * pos);
                    fadeIndex_ += rate_;
                }
                else
                {
                    mustFade_ = Fade::NO_FADE;
                }
            }
            // Gradually stop writing, fading from the input signal to the buffered
            // value.
            else if (Fade::OUT == mustFade_)
            {
                float pos = fadeIndex_ * (1.f / samplesToFade);
                if (fadeIndex_ < samplesToFade - 1)
                {
                    value = value * (1.0f - pos) + (currentValue_ * pos);
                    fadeIndex_ += rate_;
                }
                else
                {
                    mustFade_ = Fade::NO_FADE;
                    // Actually stop writing.
                    Stop(false);
                }
            }
            if (run_)
            {
                buffer_[WrapIndex(intIndex_)] = value;
            }
        }

        void ClearBuffer()
        {
            memset(buffer_, 0.f, maxBufferSamples_);
        }

        bool Buffer(float value)
        {
            buffer_[intIndex_] = value;

            // Handle end of buffer.
            if (bufferSamples_ > maxBufferSamples_ - 1)
            {
                return true;
            }

            intIndex_++;
            bufferSamples_ = intIndex_;

            return false;
        }

        void InitBuffer(int32_t bufferSamples)
        {
            bufferSamples_ = bufferSamples;
            loopLength_ = bufferSamples_;
            loopEnd_ = loopLength_ - 1;
        }

        int32_t StopBuffering()
        {
            intIndex_ = 0;
            loopLength_ = bufferSamples_;
            loopEnd_ = loopLength_ - 1;
            ResetPosition();

            return bufferSamples_;
        }

        inline Direction ToggleDirection()
        {
            direction_ = static_cast<Direction>(direction_ * -1);

            return direction_;
        }
        inline void Run(bool fade)
        {
            if (fade)
            {
                SetUpFade(Fade::IN);
            }
            else
            {
                run_ = true;
            }
        }
        inline void Stop(bool fade)
        {
            if (fade && !isStopping_)
            {
                isStopping_ = true;
                SetUpFade(Fade::OUT);
            }
            else
            {
                isStopping_ = false;
                run_ = false;
                ResetPosition();
            }
        }
        inline bool ToggleRun()
        {
            run_ = !run_;

            return run_;
        }
        inline void SetLooping(bool looping)
        {
            looping_ = looping;
        }

        inline int32_t GetBufferSamples() { return bufferSamples_; }
        inline int32_t GetLoopEnd() { return loopEnd_; }
        inline float GetRate() { return rate_; }
        inline float GetPosition() { return index_; }
        inline int32_t GetIntPosition() { return intIndex_; }
        bool IsGoingForward() { return FORWARD == direction_; }
    };
}