// Gaussian energy fading tables for QRA64
static const int glen_tab_gauss[64] = {
  2,   2,   2,   2,   2,   2,   2,   2, 
  2,   2,   2,   2,   2,   2,   2,   2, 
  3,   3,   3,   3,   3,   3,   3,   3, 
  4,   4,   4,   4,   5,   5,   5,   6, 
  6,   6,   7,   7,   8,   8,   9,  10, 
 10,  11,  12,  13,  14,  15,  17,  18, 
 19,  21,  23,  25,  27,  29,  32,  34, 
 37,  41,  44,  48,  52,  57,  62,  65
};
static const float ggauss1[2] = {
0.0296f, 0.9101f
};
static const float ggauss2[2] = {
0.0350f, 0.8954f
};
static const float ggauss3[2] = {
0.0411f, 0.8787f
};
static const float ggauss4[2] = {
0.0483f, 0.8598f
};
static const float ggauss5[2] = {
0.0566f, 0.8387f
};
static const float ggauss6[2] = {
0.0660f, 0.8154f
};
static const float ggauss7[2] = {
0.0767f, 0.7898f
};
static const float ggauss8[2] = {
0.0886f, 0.7621f
};
static const float ggauss9[2] = {
0.1017f, 0.7325f
};
static const float ggauss10[2] = {
0.1159f, 0.7012f
};
static const float ggauss11[2] = {
0.1310f, 0.6687f
};
static const float ggauss12[2] = {
0.1465f, 0.6352f
};
static const float ggauss13[2] = {
0.1621f, 0.6013f
};
static const float ggauss14[2] = {
0.1771f, 0.5674f
};
static const float ggauss15[2] = {
0.1911f, 0.5339f
};
static const float ggauss16[2] = {
0.2034f, 0.5010f
};
static const float ggauss17[3] = {
0.0299f, 0.2135f, 0.4690f
};
static const float ggauss18[3] = {
0.0369f, 0.2212f, 0.4383f
};
static const float ggauss19[3] = {
0.0454f, 0.2263f, 0.4088f
};
static const float ggauss20[3] = {
0.0552f, 0.2286f, 0.3806f
};
static const float ggauss21[3] = {
0.0658f, 0.2284f, 0.3539f
};
static const float ggauss22[3] = {
0.0766f, 0.2258f, 0.3287f
};
static const float ggauss23[3] = {
0.0869f, 0.2212f, 0.3049f
};
static const float ggauss24[3] = {
0.0962f, 0.2148f, 0.2826f
};
static const float ggauss25[4] = {
0.0351f, 0.1041f, 0.2071f, 0.2616f
};
static const float ggauss26[4] = {
0.0429f, 0.1102f, 0.1984f, 0.2420f
};
static const float ggauss27[4] = {
0.0508f, 0.1145f, 0.1890f, 0.2237f
};
static const float ggauss28[4] = {
0.0582f, 0.1169f, 0.1791f, 0.2067f
};
static const float ggauss29[5] = {
0.0289f, 0.0648f, 0.1176f, 0.1689f, 0.1908f
};
static const float ggauss30[5] = {
0.0351f, 0.0703f, 0.1168f, 0.1588f, 0.1760f
};
static const float ggauss31[5] = {
0.0411f, 0.0745f, 0.1146f, 0.1488f, 0.1623f
};
static const float ggauss32[6] = {
0.0246f, 0.0466f, 0.0773f, 0.1115f, 0.1390f, 0.1497f
};
static const float ggauss33[6] = {
0.0297f, 0.0512f, 0.0788f, 0.1075f, 0.1295f, 0.1379f
};
static const float ggauss34[6] = {
0.0345f, 0.0549f, 0.0791f, 0.1029f, 0.1205f, 0.1270f
};
static const float ggauss35[7] = {
0.0240f, 0.0387f, 0.0575f, 0.0784f, 0.0979f, 0.1118f, 0.1169f
};
static const float ggauss36[7] = {
0.0281f, 0.0422f, 0.0590f, 0.0767f, 0.0926f, 0.1037f, 0.1076f
};
static const float ggauss37[8] = {
0.0212f, 0.0318f, 0.0449f, 0.0596f, 0.0744f, 0.0872f, 0.0960f, 0.0991f
};
static const float ggauss38[8] = {
0.0247f, 0.0348f, 0.0467f, 0.0593f, 0.0716f, 0.0819f, 0.0887f, 0.0911f
};
static const float ggauss39[9] = {
0.0199f, 0.0278f, 0.0372f, 0.0476f, 0.0584f, 0.0684f, 0.0766f, 0.0819f, 
0.0838f
};
static const float ggauss40[10] = {
0.0166f, 0.0228f, 0.0303f, 0.0388f, 0.0478f, 0.0568f, 0.0649f, 0.0714f, 
0.0756f, 0.0771f
};
static const float ggauss41[10] = {
0.0193f, 0.0254f, 0.0322f, 0.0397f, 0.0474f, 0.0548f, 0.0613f, 0.0664f, 
0.0697f, 0.0709f
};
static const float ggauss42[11] = {
0.0168f, 0.0217f, 0.0273f, 0.0335f, 0.0399f, 0.0464f, 0.0524f, 0.0576f, 
0.0617f, 0.0643f, 0.0651f
};
static const float ggauss43[12] = {
0.0151f, 0.0191f, 0.0237f, 0.0288f, 0.0342f, 0.0396f, 0.0449f, 0.0498f, 
0.0540f, 0.0572f, 0.0592f, 0.0599f
};
static const float ggauss44[13] = {
0.0138f, 0.0171f, 0.0210f, 0.0252f, 0.0297f, 0.0343f, 0.0388f, 0.0432f, 
0.0471f, 0.0504f, 0.0529f, 0.0545f, 0.0550f
};
static const float ggauss45[14] = {
0.0128f, 0.0157f, 0.0189f, 0.0224f, 0.0261f, 0.0300f, 0.0339f, 0.0377f, 
0.0412f, 0.0444f, 0.0470f, 0.0489f, 0.0501f, 0.0505f
};
static const float ggauss46[15] = {
0.0121f, 0.0146f, 0.0173f, 0.0202f, 0.0234f, 0.0266f, 0.0299f, 0.0332f, 
0.0363f, 0.0391f, 0.0416f, 0.0437f, 0.0452f, 0.0461f, 0.0464f
};
static const float ggauss47[17] = {
0.0097f, 0.0116f, 0.0138f, 0.0161f, 0.0186f, 0.0212f, 0.0239f, 0.0267f, 
0.0294f, 0.0321f, 0.0346f, 0.0369f, 0.0389f, 0.0405f, 0.0417f, 0.0424f, 
0.0427f
};
static const float ggauss48[18] = {
0.0096f, 0.0113f, 0.0131f, 0.0151f, 0.0172f, 0.0194f, 0.0217f, 0.0241f, 
0.0264f, 0.0287f, 0.0308f, 0.0329f, 0.0347f, 0.0362f, 0.0375f, 0.0384f, 
0.0390f, 0.0392f
};
static const float ggauss49[19] = {
0.0095f, 0.0110f, 0.0126f, 0.0143f, 0.0161f, 0.0180f, 0.0199f, 0.0219f, 
0.0239f, 0.0258f, 0.0277f, 0.0294f, 0.0310f, 0.0325f, 0.0337f, 0.0347f, 
0.0354f, 0.0358f, 0.0360f
};
static const float ggauss50[21] = {
0.0083f, 0.0095f, 0.0108f, 0.0122f, 0.0136f, 0.0152f, 0.0168f, 0.0184f, 
0.0201f, 0.0217f, 0.0234f, 0.0250f, 0.0265f, 0.0279f, 0.0292f, 0.0303f, 
0.0313f, 0.0320f, 0.0326f, 0.0329f, 0.0330f
};
static const float ggauss51[23] = {
0.0074f, 0.0084f, 0.0095f, 0.0106f, 0.0118f, 0.0131f, 0.0144f, 0.0157f, 
0.0171f, 0.0185f, 0.0199f, 0.0213f, 0.0227f, 0.0240f, 0.0252f, 0.0263f, 
0.0273f, 0.0282f, 0.0290f, 0.0296f, 0.0300f, 0.0303f, 0.0303f
};
static const float ggauss52[25] = {
0.0068f, 0.0076f, 0.0085f, 0.0094f, 0.0104f, 0.0115f, 0.0126f, 0.0137f, 
0.0149f, 0.0160f, 0.0172f, 0.0184f, 0.0196f, 0.0207f, 0.0218f, 0.0228f, 
0.0238f, 0.0247f, 0.0255f, 0.0262f, 0.0268f, 0.0273f, 0.0276f, 0.0278f, 
0.0279f
};
static const float ggauss53[27] = {
0.0063f, 0.0070f, 0.0078f, 0.0086f, 0.0094f, 0.0103f, 0.0112f, 0.0121f, 
0.0131f, 0.0141f, 0.0151f, 0.0161f, 0.0170f, 0.0180f, 0.0190f, 0.0199f, 
0.0208f, 0.0216f, 0.0224f, 0.0231f, 0.0237f, 0.0243f, 0.0247f, 0.0251f, 
0.0254f, 0.0255f, 0.0256f
};
static const float ggauss54[29] = {
0.0060f, 0.0066f, 0.0072f, 0.0079f, 0.0086f, 0.0093f, 0.0101f, 0.0109f, 
0.0117f, 0.0125f, 0.0133f, 0.0142f, 0.0150f, 0.0159f, 0.0167f, 0.0175f, 
0.0183f, 0.0190f, 0.0197f, 0.0204f, 0.0210f, 0.0216f, 0.0221f, 0.0225f, 
0.0228f, 0.0231f, 0.0233f, 0.0234f, 0.0235f
};
static const float ggauss55[32] = {
0.0053f, 0.0058f, 0.0063f, 0.0068f, 0.0074f, 0.0080f, 0.0086f, 0.0093f, 
0.0099f, 0.0106f, 0.0113f, 0.0120f, 0.0127f, 0.0134f, 0.0141f, 0.0148f, 
0.0155f, 0.0162f, 0.0168f, 0.0174f, 0.0180f, 0.0186f, 0.0191f, 0.0196f, 
0.0201f, 0.0204f, 0.0208f, 0.0211f, 0.0213f, 0.0214f, 0.0215f, 0.0216f
};
static const float ggauss56[34] = {
0.0052f, 0.0056f, 0.0060f, 0.0065f, 0.0070f, 0.0075f, 0.0080f, 0.0086f, 
0.0091f, 0.0097f, 0.0103f, 0.0109f, 0.0115f, 0.0121f, 0.0127f, 0.0133f, 
0.0138f, 0.0144f, 0.0150f, 0.0155f, 0.0161f, 0.0166f, 0.0170f, 0.0175f, 
0.0179f, 0.0183f, 0.0186f, 0.0189f, 0.0192f, 0.0194f, 0.0196f, 0.0197f, 
0.0198f, 0.0198f
};
static const float ggauss57[37] = {
0.0047f, 0.0051f, 0.0055f, 0.0058f, 0.0063f, 0.0067f, 0.0071f, 0.0076f, 
0.0080f, 0.0085f, 0.0090f, 0.0095f, 0.0100f, 0.0105f, 0.0110f, 0.0115f, 
0.0120f, 0.0125f, 0.0130f, 0.0134f, 0.0139f, 0.0144f, 0.0148f, 0.0152f, 
0.0156f, 0.0160f, 0.0164f, 0.0167f, 0.0170f, 0.0173f, 0.0175f, 0.0177f, 
0.0179f, 0.0180f, 0.0181f, 0.0181f, 0.0182f
};
static const float ggauss58[41] = {
0.0041f, 0.0044f, 0.0047f, 0.0050f, 0.0054f, 0.0057f, 0.0060f, 0.0064f, 
0.0068f, 0.0072f, 0.0076f, 0.0080f, 0.0084f, 0.0088f, 0.0092f, 0.0096f, 
0.0101f, 0.0105f, 0.0109f, 0.0113f, 0.0117f, 0.0121f, 0.0125f, 0.0129f, 
0.0133f, 0.0137f, 0.0140f, 0.0144f, 0.0147f, 0.0150f, 0.0153f, 0.0155f, 
0.0158f, 0.0160f, 0.0162f, 0.0163f, 0.0164f, 0.0165f, 0.0166f, 0.0167f, 
0.0167f
};
static const float ggauss59[44] = {
0.0039f, 0.0042f, 0.0044f, 0.0047f, 0.0050f, 0.0053f, 0.0056f, 0.0059f, 
0.0062f, 0.0065f, 0.0068f, 0.0072f, 0.0075f, 0.0079f, 0.0082f, 0.0086f, 
0.0089f, 0.0093f, 0.0096f, 0.0100f, 0.0104f, 0.0107f, 0.0110f, 0.0114f, 
0.0117f, 0.0120f, 0.0124f, 0.0127f, 0.0130f, 0.0132f, 0.0135f, 0.0138f, 
0.0140f, 0.0142f, 0.0144f, 0.0146f, 0.0148f, 0.0149f, 0.0150f, 0.0151f, 
0.0152f, 0.0153f, 0.0153f, 0.0153f
};
static const float ggauss60[48] = {
0.0036f, 0.0038f, 0.0040f, 0.0042f, 0.0044f, 0.0047f, 0.0049f, 0.0052f, 
0.0055f, 0.0057f, 0.0060f, 0.0063f, 0.0066f, 0.0068f, 0.0071f, 0.0074f, 
0.0077f, 0.0080f, 0.0083f, 0.0086f, 0.0089f, 0.0092f, 0.0095f, 0.0098f, 
0.0101f, 0.0104f, 0.0107f, 0.0109f, 0.0112f, 0.0115f, 0.0117f, 0.0120f, 
0.0122f, 0.0124f, 0.0126f, 0.0128f, 0.0130f, 0.0132f, 0.0134f, 0.0135f, 
0.0136f, 0.0137f, 0.0138f, 0.0139f, 0.0140f, 0.0140f, 0.0140f, 0.0140f
};
static const float ggauss61[52] = {
0.0033f, 0.0035f, 0.0037f, 0.0039f, 0.0041f, 0.0043f, 0.0045f, 0.0047f, 
0.0049f, 0.0051f, 0.0053f, 0.0056f, 0.0058f, 0.0060f, 0.0063f, 0.0065f, 
0.0068f, 0.0070f, 0.0073f, 0.0075f, 0.0078f, 0.0080f, 0.0083f, 0.0085f, 
0.0088f, 0.0090f, 0.0093f, 0.0095f, 0.0098f, 0.0100f, 0.0102f, 0.0105f, 
0.0107f, 0.0109f, 0.0111f, 0.0113f, 0.0115f, 0.0116f, 0.0118f, 0.0120f, 
0.0121f, 0.0122f, 0.0124f, 0.0125f, 0.0126f, 0.0126f, 0.0127f, 0.0128f, 
0.0128f, 0.0129f, 0.0129f, 0.0129f
};
static const float ggauss62[57] = {
0.0030f, 0.0031f, 0.0033f, 0.0034f, 0.0036f, 0.0038f, 0.0039f, 0.0041f, 
0.0043f, 0.0045f, 0.0047f, 0.0048f, 0.0050f, 0.0052f, 0.0054f, 0.0056f, 
0.0058f, 0.0060f, 0.0063f, 0.0065f, 0.0067f, 0.0069f, 0.0071f, 0.0073f, 
0.0075f, 0.0077f, 0.0080f, 0.0082f, 0.0084f, 0.0086f, 0.0088f, 0.0090f, 
0.0092f, 0.0094f, 0.0096f, 0.0097f, 0.0099f, 0.0101f, 0.0103f, 0.0104f, 
0.0106f, 0.0107f, 0.0108f, 0.0110f, 0.0111f, 0.0112f, 0.0113f, 0.0114f, 
0.0115f, 0.0116f, 0.0116f, 0.0117f, 0.0117f, 0.0118f, 0.0118f, 0.0118f, 
0.0118f
};
static const float ggauss63[62] = {
0.0027f, 0.0029f, 0.0030f, 0.0031f, 0.0032f, 0.0034f, 0.0035f, 0.0037f, 
0.0038f, 0.0040f, 0.0041f, 0.0043f, 0.0045f, 0.0046f, 0.0048f, 0.0049f, 
0.0051f, 0.0053f, 0.0055f, 0.0056f, 0.0058f, 0.0060f, 0.0062f, 0.0063f, 
0.0065f, 0.0067f, 0.0069f, 0.0071f, 0.0072f, 0.0074f, 0.0076f, 0.0078f, 
0.0079f, 0.0081f, 0.0083f, 0.0084f, 0.0086f, 0.0088f, 0.0089f, 0.0091f, 
0.0092f, 0.0094f, 0.0095f, 0.0096f, 0.0098f, 0.0099f, 0.0100f, 0.0101f, 
0.0102f, 0.0103f, 0.0104f, 0.0105f, 0.0105f, 0.0106f, 0.0107f, 0.0107f, 
0.0108f, 0.0108f, 0.0108f, 0.0108f, 0.0109f, 0.0109f
};
static const float ggauss64[65] = {
0.0028f, 0.0029f, 0.0030f, 0.0031f, 0.0032f, 0.0034f, 0.0035f, 0.0036f, 
0.0037f, 0.0039f, 0.0040f, 0.0041f, 0.0043f, 0.0044f, 0.0046f, 0.0047f, 
0.0048f, 0.0050f, 0.0051f, 0.0053f, 0.0054f, 0.0056f, 0.0057f, 0.0059f, 
0.0060f, 0.0062f, 0.0063f, 0.0065f, 0.0066f, 0.0068f, 0.0069f, 0.0071f, 
0.0072f, 0.0074f, 0.0075f, 0.0077f, 0.0078f, 0.0079f, 0.0081f, 0.0082f, 
0.0083f, 0.0084f, 0.0086f, 0.0087f, 0.0088f, 0.0089f, 0.0090f, 0.0091f, 
0.0092f, 0.0093f, 0.0094f, 0.0094f, 0.0095f, 0.0096f, 0.0097f, 0.0097f, 
0.0098f, 0.0098f, 0.0099f, 0.0099f, 0.0099f, 0.0099f, 0.0100f, 0.0100f, 
0.0100f
};
static const float *gptr_tab_gauss[64] = {
ggauss1, ggauss2, ggauss3, ggauss4, 
ggauss5, ggauss6, ggauss7, ggauss8, 
ggauss9, ggauss10, ggauss11, ggauss12, 
ggauss13, ggauss14, ggauss15, ggauss16, 
ggauss17, ggauss18, ggauss19, ggauss20, 
ggauss21, ggauss22, ggauss23, ggauss24, 
ggauss25, ggauss26, ggauss27, ggauss28, 
ggauss29, ggauss30, ggauss31, ggauss32, 
ggauss33, ggauss34, ggauss35, ggauss36, 
ggauss37, ggauss38, ggauss39, ggauss40, 
ggauss41, ggauss42, ggauss43, ggauss44, 
ggauss45, ggauss46, ggauss47, ggauss48, 
ggauss49, ggauss50, ggauss51, ggauss52, 
ggauss53, ggauss54, ggauss55, ggauss56, 
ggauss57, ggauss58, ggauss59, ggauss60, 
ggauss61, ggauss62, ggauss63, ggauss64
};
