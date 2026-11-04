/*
 * keysightStats.c - aSub routine computing statistics of a READ? array
 *
 * Inputs
 *   A : readings (DOUBLE[NOA]); NEA = number of readings returned
 * Outputs
 *   VALA last reading      VALB mean        VALC std deviation (sample)
 *   VALD min               VALE max         VALF number of valid readings
 *   VALG overload flag (1 if any reading is +/-9.9E+37)
 *
 * Overloaded readings are excluded from mean/std/min/max; if no valid
 * reading is left, the statistics are NaN and the record alarms.
 */

#include <math.h>

#include <aSubRecord.h>
#include <alarm.h>
#include <recGbl.h>
#include <registryFunction.h>
#include <epicsExport.h>

#define DMM_OVERLOAD 9.0e37

static long keysightStats(aSubRecord *prec)
{
    const double *a = (const double *) prec->a;
    epicsUInt32 n = prec->nea;
    epicsUInt32 i, nvalid = 0;
    epicsInt32 ovld = 0;
    double sum = 0.0, sum2 = 0.0, mean, mn = 0.0, mx = 0.0, stdev = 0.0;
    double last = NAN;

    if (n > prec->noa)
        n = prec->noa;

    for (i = 0; i < n; i++) {
        double v = a[i];
        if (!isfinite(v) || fabs(v) >= DMM_OVERLOAD) {
            ovld = 1;
            continue;
        }
        if (nvalid == 0) {
            mn = mx = v;
        } else {
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        sum += v;
        nvalid++;
    }
    if (n > 0)
        last = a[n - 1];

    if (nvalid > 0) {
        mean = sum / nvalid;
        for (i = 0; i < n; i++) {
            double v = a[i];
            if (isfinite(v) && fabs(v) < DMM_OVERLOAD)
                sum2 += (v - mean) * (v - mean);
        }
        if (nvalid > 1)
            stdev = sqrt(sum2 / (nvalid - 1));
    } else {
        mean = mn = mx = stdev = NAN;
        recGblSetSevr(prec, CALC_ALARM, INVALID_ALARM);
    }

    *(double *) prec->vala = last;
    *(double *) prec->valb = mean;
    *(double *) prec->valc = stdev;
    *(double *) prec->vald = mn;
    *(double *) prec->vale = mx;
    *(epicsInt32 *) prec->valf = (epicsInt32) nvalid;
    *(epicsInt32 *) prec->valg = ovld;
    return 0;
}

epicsRegisterFunction(keysightStats);
