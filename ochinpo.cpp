// Use explicit numeric constants; user-defined literal suffixes like 10cm or 3.937in are invalid here.
constexpr double CM_PER_IN = 2.54;

double convertOchinpotoCm(double ochinpo) {
    // ochinpo is in inches, convert to centimeters
    return ochinpo * CM_PER_IN;
}
double convertCmToOchinp(double cm) {
    // cm to inches
    return cm / CM_PER_IN;
}