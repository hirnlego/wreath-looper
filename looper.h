#pragma once

#include "head.h"
#include <cstdint>

namespace wreath
{
    class Looper
    {
    public:
        Looper() {}
        ~Looper() {}

        void Init(int32_t sampleRate, float *buffer, int32_t maxBufferSamples);
        void Reset();
        void ClearBuffer();
        void StopBuffering();
        void SetReadRate(float rate);
        void SetWriteRate(float rate);
        void SetLoopLength(int32_t length);
        void SetMovement(Movement movement);
        void SetLooping(bool looping);
        bool Buffer(float value);
        void SetReadPos(float position);
        void SetWritePos(float position);
        float Read();
        void Write(float value);
        void UpdateReadPos();
        void UpdateWritePos();
        bool HandleFade();
        bool Start();
        bool Stop();
        bool Restart(bool resetPosition);
        void SetLoopStart(int32_t pos);
        int32_t GetRandomPosition();
        void SetLoopEnd(int32_t pos);
        void SetDirection(Direction direction);
        void SetRunStatus(RunStatus status)
        {
            heads_[READ].SetRunStatus(status);
            runStatus_ = status;
        }
        void ToggleDirection();
        void SetWriting(float amount);

        inline int32_t GetBufferSamples() { return bufferSamples_; }
        inline float GetBufferSeconds() { return bufferSeconds_; }

        inline int32_t GetLoopStart() { return loopStart_; }
        inline float GetLoopStartSeconds() { return loopStartSeconds_; }

        inline int32_t GetLoopEnd() { return loopEnd_; }

        inline int32_t GetLoopLength() { return loopLength_; }
        inline float GetLoopLengthSeconds() { return loopLengthSeconds_; }

        inline float GetReadPos() { return readPos_; }
        inline float GetReadPosSeconds() { return readPosSeconds_; }
        inline float GetNextReadPos() { return nextReadPos_; }

        inline int32_t GetWritePos() { return writePos_; }

        inline float GetReadRate() { return readRate_; }
        inline float GetWriteRate() { return writeRate_; }
        inline int32_t GetSampleRateSpeed() { return sampleRateSpeed_; }

        inline Movement GetMovement() { return movement_; }
        inline Direction GetDirection() { return direction_; }
        inline RunStatus GetRunStatus() { return runStatus_; }
        inline bool IsDrunkMovement() { return Movement::DRUNK == movement_; }
        inline bool IsGoingForward() { return Direction::FORWARD == direction_; }

        inline void SetReading(bool active) { readingActive_ = active; }

        inline int32_t GetCrossPoint() { return crossPoint_; }
        inline bool CrossPointFound() { return crossPointFound_; }

    private:
        void CalculateHeadsDistance();
        void CalculateCrossPoint();

        float *buffer_{};           // The buffer
        float bufferSeconds_{};     // Written buffer length in seconds
        float readPos_{};           // The read position
        float readPosSeconds_{};    // Read position in seconds
        float nextReadPos_{};       // Next read position
        float loopStartSeconds_{};  // Start of the loop in seconds
        float loopLengthSeconds_{}; // Length of the loop in seconds
        float readRate_{};         // Speed multiplier
        float writeRate_{};         // Speed multiplier
        float readSpeed_{};         // Actual read speed
        float writeSpeed_{};        // Actual write speed
        int32_t bufferSamples_{};    // The written buffer length in samples
        int32_t writePos_{};         // The write position
        int32_t loopStart_{};        // Loop start position
        int32_t loopEnd_{};          // Loop end position
        int32_t loopLength_{};       // Length of the loop in samples
        int32_t headsDistance_{};
        int32_t sampleRate_{}; // The sample rate
        Direction direction_{};
        int32_t crossPoint_{};
        bool crossPointFound_{};
        bool readingActive_{true};
        bool writingActive_{true};
        int32_t sampleRateSpeed_{};
        bool looping_{};
        bool isRestarting_{};

        Head heads_[2]{{Type::READ}, {Type::WRITE}};

        RunStatus runStatus_{};
        Movement movement_{}; // The current movement type of the looper
    };
} // namespace wreath