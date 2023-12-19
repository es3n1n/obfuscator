#pragma once
#include "logger.hpp"
#include "util/stopwatch.hpp"

namespace util {
    /// \brief Progress-bar object
    class Progress {
    public:
        DEFAULT_DTOR(Progress);
        NON_COPYABLE(Progress);

        /// \brief
        /// \param title Title like "Obfuscating function main"
        /// \param num_steps Number of total steps for a progress
        explicit Progress(const std::string_view title, const std::size_t num_steps): title_(title), steps_(num_steps) {
            step();
        }

        /// \brief Do a step and update the progress msg
        void step() {
            /// Increment the step
            ++step_;

            /// Calculate current percentage
            const auto percents = static_cast<float>(step_) / static_cast<float>(steps_);

            /// Format msg with elapsed time if needed
            std::string msg = std::format("{}: {}%", title_, static_cast<std::size_t>(percents * 100));
            if (step_ == steps_) {
                msg += std::format(" took {}", stopwatch_.elapsed());
            }

            /// Print it
            logger::info(msg.c_str());
        }

    private:
        /// \brief Stopwatch
        Stopwatch stopwatch_ = {};
        /// \brief Progress bar title
        std::string title_ = {};
        /// \brief Total number of steps
        std::size_t steps_ = {};
        /// \brief Current step
        std::ptrdiff_t step_ = -1; // we start at -1 and it will automatically increment it to 0
    };
} // namespace util
