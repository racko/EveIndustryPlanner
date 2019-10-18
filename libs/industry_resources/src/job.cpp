#include "job.h"

#include <infinity.h>

Job::Job(Resources i, Resources o, double c, std::string n, std::string fn, double l, double u)
    : inputs(std::move(i)), outputs(std::move(o)), cost(c), lower_limit(l), upper_limit(u), name(std::move(n)),
      fullName(std::move(fn)) {}

Job::Job(Resources i, Resources o, double c, std::string n, std::string fn, double l)
    : Job{std::move(i), std::move(o), c, std::move(n), std::move(fn), l, infinity} {}

Job::~Job() = default;
