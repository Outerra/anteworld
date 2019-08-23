
#include "cubeface.h"

namespace ot {

double CUBEFACE_M = 0.2761423749;
bool QSC_OTC = false;

////////////////////////////////////////////////////////////////////////////////
const double& cubeface_qsc_otc( bool qsc_otc )
{
    const_cast<bool&>(QSC_OTC) = qsc_otc;
    const_cast<double&>(CUBEFACE_M) = QSC_OTC
        ? 0.5/(M_SQRT2 - 1) - 1      //OT conformal projection, best ratio preservation
        : 0.2761423749;              //old OT approximation

    return CUBEFACE_M;
}

} //namespace ot
