//===Variable Declaration===============================================================
float passThres = 90.123;
float LPThres = 90.123;
int indHS, indLP,indMS, indTO, indZC, j = 0;
float maxA, minA = 0.0;

//===Heel-Strike Detection==============================================================
if(wz >= passThres && wz_prev <= passThres){
countH = countH + 1;
flagForce = 0;
}
if(wz <= passThres && wz_prev >= passThres){
countH = countH + 1;
}
if(wz_prev <= wz_pprev && wz_prev <= wz && wz_prev < 0 && countH >0){
if(countH%2 == 0){
hsTime = t;
countH = 0;
flagLP = 1;
flagMS = 1;
flagZC = 1;
thisHS = wz_prev;
indHS = 1;
hsms = 200*(hsTime - msTime);
rtAngDS = (180/pi) * atan(  (wz_prev - thisMS) / (hsTime - msTime) );

}
}

//==Zero Crossing 1 Detection=============================================================
if(wz_prev  < 0 && wz > 0 && flagZC == 1 && t-hsTime > 0.04){
thisZC = wz;
flagZC = 0;
indZC = 1;
}

//===Lambda-Peak Detection=============================================================
if(wz_prev >= wz_pprev && wz_prev >= wz && wz_pprev > 0.8*thisHS && wz_pprev < passThres && flagLP == 1 && t-hsTime > 0.18){
lpTime = t;
flagLP = 0;
flagTO = 1;
thisLP = wz_prev;
indLP = 1;
flagForce = 1;
flagZC = 0;
//======================Classifier Algorithm Invoke at Lambda-Peak Detection ======================
if (wz > 6.23){
CS = 4*100 + 400;                                        // Upstairs
}     
if( hsms > calDS  ){                                 
CS = 3*100 + 400;                                         // Downstairs
}               
else if(wz < 0){
//if( (prevCS == 800 || prevCS == 700 || prevCS == 400)  ){
//CS = -1*100 + 400;                                       // Transition
//}
if(thisHS < LL){
CS = 1*100 + 400;                                         // Overground
}
}
}

//===Toe-Off Detection===
if(wz_prev <= wz_pprev && wz_prev <= wz && wz_prev < 0.9*thisHS && flagTO == 1){
if (t - lpTime > 0.21) {
toTime = t;
flagTO = 0;
//flagMS = 1;
thisTO = wz_prev;
indTO = 1;
}
}

//===MidSwing(MS) Detection===
if(wz_prev >= wz_pprev && wz_prev >= wz && wz_prev > LPThres && flagMS == 1) {
msTime = t;
flagMS = 0;
thisMS = wz_prev;
indMS = 1;
}

//===Stationary Detection===
for(j = 0; j < 15; j++){
if(maxA < A[j]){
maxA = A[j];
}
if(minA > A[j]){
minA = A[j];
}
}
if(abs(maxA - minA) <= 3 && maxA > -8 && minA > -8){
CS = 0*100 + 400;                        // Stationary
}

prevCS = CS;
wz_pprev = wz_prev;
wz_prev = wz;