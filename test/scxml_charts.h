#pragma once
/*
 * Charts ported from the W3C SCXML 1.0 Implementation Report (IRP) test suite,
 * https://www.w3.org/Voice/2013/scxml-irp/, see scxml_charts.c.
 *
 * Like the IRP tests, every chart ends in its PASS state on success. External events to run
 * after hsm4c_init() are listed in the tests.
 */

#include <hsm4c/hsm4c.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief State ID of the pass state in every chart. */
#define SCXML_PASS 1
/** \brief State ID of the fail state in every chart. */
#define SCXML_FAIL 2

/** \brief Context for the charts: the IRP tests' counters (conf:incrementID). */
struct scxml_ctx {
  int var[4];
};

/** \brief External events. */
enum scxml_external_event { SCXML_EXTERNAL = 1000 };

extern hsm4c_chart_t const scxml_387;
extern hsm4c_chart_t const scxml_388;
extern hsm4c_chart_t const scxml_403a;
extern hsm4c_chart_t const scxml_407;
extern hsm4c_chart_t const scxml_411;
extern hsm4c_chart_t const scxml_419;
extern hsm4c_chart_t const scxml_421;
extern hsm4c_chart_t const scxml_503;
extern hsm4c_chart_t const scxml_505;
extern hsm4c_chart_t const scxml_506;
extern hsm4c_chart_t const scxml_580;

#ifdef __cplusplus
}
#endif
