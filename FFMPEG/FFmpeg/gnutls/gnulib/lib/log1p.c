/* Natural logarithm of 1 plus argument.
   Copyright (C) 2012-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <math.h>

double
log1p (double x)
{
  if (isnand (x))
    return x;

  if (x <= -1.0)
    {
      if (x == -1.0)
        /* Return -Infinity.  */
        return - HUGE_VAL;
      else
        {
          /* Return NaN.  */
#if defined _MSC_VER || (defined __sgi && !defined __GNUC__)
          static double zero;
          return zero / zero;
#else
          return 0.0 / 0.0;
#endif
        }
    }

  if (x < -0.5 || x > 1.0)
    return log (1.0 + x);
  /* Here -0.5 <= x <= 1.0.  */

  if (x == 0.0)
    /* Return a zero with the same sign as x.  */
    return x;

  /* Decompose x into
       1 + x = (1 + m/256) * (1 + y)
     where
       m is an integer, -128 <= m <= 256,
       y is a number, |y| <= 1/256.
     y is computed as
       y = (256 * x - m) / (256 + m).
     Then
       log(1+x) = log(m/256) + log(1+y)
     The first summand is a table lookup.
     The second summand is computed
       - either through the power series
           log(1+y) = y
                      - 1/2 * y^2
                      + 1/3 * y^3
                      - 1/4 * y^4
                      + 1/5 * y^5
                      - 1/6 * y^6
                      + 1/7 * y^7
                      - 1/8 * y^8
                      + 1/9 * y^9
                      - 1/10 * y^10
                      + 1/11 * y^11
                      - 1/12 * y^12
                      + 1/13 * y^13
                      - 1/14 * y^14
                      + 1/15 * y^15
                      - ...
       - or as log(1+y) = log((1+z)/(1-z)) = 2 * atanh(z)
         where z = y/(2+y)
         and atanh(z) is computed through its power series:
           atanh(z) = z
                      + 1/3 * z^3
                      + 1/5 * z^5
                      + 1/7 * z^7
                      + 1/9 * z^9
                      + 1/11 * z^11
                      + 1/13 * z^13
                      + 1/15 * z^15
                      + ...
         Since |z| <= 1/511 < 0.002, the relative contribution of the z^9
         term is < 1/9*0.002^8 < 2^-60 <= 2^-DBL_MANT_DIG, therefore we
         can truncate the series after the z^7 term.  */

  {
    double m = round (x * 256.0);
    double y = ((x * 256.0) - m) / (m + 256.0);
    double z = y / (2.0 + y);

/* Coefficients of the power series for atanh(z).  */
#define ATANH_COEFF_1  1.0
#define ATANH_COEFF_3  0.333333333333333333333333333333333333334
#define ATANH_COEFF_5  0.2
#define ATANH_COEFF_7  0.142857142857142857142857142857142857143
#define ATANH_COEFF_9  0.1111111111111111111111111111111111111113
#define ATANH_COEFF_11 0.090909090909090909090909090909090909091
#define ATANH_COEFF_13 0.076923076923076923076923076923076923077
#define ATANH_COEFF_15 0.066666666666666666666666666666666666667

    double z2 = z * z;
    double atanh_z =
      (((ATANH_COEFF_7
         * z2 + ATANH_COEFF_5)
        * z2 + ATANH_COEFF_3)
       * z2 + ATANH_COEFF_1)
      * z;

    /* log_table[i] = log((i + 128) / 256).
       Computed in GNU clisp through
         (setf (long-float-digits) 128)
         (setq a 0L0)
         (setf (long-float-digits) 256)
         (dotimes (i 385)
           (format t "        ~D,~%"
                   (float (log (* (/ (+ i 128) 256) 1L0)) a)))  */
    static const double log_table[385] =
      {
        -0.693147180559945309417232121458176568075,
        -0.6853650401178903604697692213970398044,
        -0.677642994023980055266378075415729732197,
        -0.669980121278410931188432960495886651496,
        -0.662375521893191621046203913861404403985,
        -0.65482831625780871022347679633437927773,
        -0.647337644528651106250552853843513225963,
        -0.639902666041133026551361927671647791137,
        -0.632522558743510466836625989417756304788,
        -0.625196518651437560022666843685547154042,
        -0.617923759322357783718626781474514153438,
        -0.61070351134887071814907205278986876216,
        -0.60353502187025817679728065207969203929,
        -0.59641755410139419712166106497071313106,
        -0.58935038687830174459117031769420187977,
        -0.582332814219655195222425952134964639978,
        -0.575364144903561854878438011987654863008,
        -0.568443702058988073553825606077313299585,
        -0.561570822771226036828515992768693405624,
        -0.554744857700826173731906247856527380683,
        -0.547965170715447412135297057717612244552,
        -0.541231138534103334345428696561292056747,
        -0.534542150383306725323860946832334992828,
        -0.527897607664638146541620672180936254347,
        -0.52129692363328608707713317540302930314,
        -0.514739523087127012297831879947234599722,
        -0.50822484206593331675332852879892694707,
        -0.50175232756031585480793331389686769463,
        -0.495321437230025429054660050261215099,
        -0.488931639131254417913411735261937295862,
        -0.482582411452595671747679308725825054355,
        -0.476273242259330949798142595713829069596,
        -0.470003629245735553650937031148342064701,
        -0.463773079495099479425751396412036696525,
        -0.457581109247178400339643902517133157939,
        -0.451427243672800141272924605544662667972,
        -0.445311016655364052636629355711651820077,
        -0.43923197057898186527990882355156990061,
        -0.4331896561230192424451526269158655235,
        -0.427183632062807368078106194920633178807,
        -0.421213465076303550585562626925177406092,
        -0.415278729556489003230882088534775334993,
        -0.409379007429300711070330899107921801414,
        -0.403513887976902632538339065932507598071,
        -0.397682967666109433030550215403212372894,
        -0.391885849981783528404356583224421075418,
        -0.386122145265033447342107580922798666387,
        -0.380391470556048421030985561769857535915,
        -0.374693449441410693606984907867576972481,
        -0.369027711905733333326561361023189215893,
        -0.363393894187477327602809309537386757124,
        -0.357791638638807479160052541644010369001,
        -0.352220593589352099112142921677820359633,
        -0.346680413213736728498769933032403617363,
        -0.341170757402767124761784665198737642087,
        -0.33569129163814153519122263131727209364,
        -0.330241686870576856279407775480686721935,
        -0.324821619401237656369001967407777741178,
        -0.31943077076636122859621528874235306143,
        -0.314068827624975851026378775827156709194,
        -0.308735481649613269682442058976885699557,
        -0.303430429419920096046768517454655701024,
        -0.298153372319076331310838085093194799765,
        -0.292904016432932602487907019463045397996,
        -0.287682072451780927439219005993827431504,
        -0.282487255574676923482925918282353780414,
        -0.277319285416234343803903228503274262719,
        -0.272177885915815673288364959951380595626,
        -0.267062785249045246292687241862699949179,
        -0.261973715741573968558059642502581569596,
        -0.256910413785027239068190798397055267412,
        -0.251872619755070079927735679796875342712,
        -0.2468600779315257978846419408385075613265,
        -0.24187253642048672427253973837916408939,
        -0.2369097470783577150364265832942468196375,
        -0.2319714654377751430492321958603212094726,
        -0.2270574506353460848586128739534071682175,
        -0.222167465341154296870334265401817316702,
        -0.2173012756899813951520225351537951559,
        -0.212458651214193401740613666010165016867,
        -0.2076393647782445016154410442673876674964,
        -0.202843192514751471266885961812429707545,
        -0.1980699137620937948192675366153429027185,
        -0.193319311003495979595900706211132426563,
        -0.188591169807550022358923589720001638093,
        -0.183885278770137362613157202229852743197,
        -0.179201429457710992616226033183958974965,
        -0.174539416351899677264255125093377869519,
        -0.169899036795397472900424896523305726435,
        -0.165280090939102924303339903679875604517,
        -0.160682381690473465543308397998034325468,
        -0.156105714663061654850502877304344269052,
        -0.1515498981272009378406898175577424691056,
        -0.1470147429618096590348349122269674042104,
        -0.142500062607283030157283942253263107981,
        -0.1380056730194437167017517619422725179055,
        -0.1335313926245226231463436209313499745895,
        -0.129077042275142343345847831367985856258,
        -0.124642445207276597338493356591214304499,
        -0.1202274269981598003244753948319154994493,
        -0.115831815525121705099120059938680166568,
        -0.1114554409253228268966213677328042273655,
        -0.1070981355563671005131126851708522185606,
        -0.1027597339577689347753154133345778104976,
        -0.098440072813252519902888574928971234883,
        -0.094138990913861910035632096996525066015,
        -0.0898563291218610470766469347968659624282,
        -0.0855919303354035139161469686670511961825,
        -0.0813456394539524058873423550293617843895,
        -0.077117303344431289769666193261475917783,
        -0.072906770808087780565737488890929711303,
        -0.0687138925480518083746933774035034481663,
        -0.064538521137571171672923915683992928129,
        -0.0603805109889074798714456529545968095868,
        -0.0562397183228760777967376942769773768851,
        -0.0521160011390140183616307870527840213665,
        -0.0480092191863606077520036253234446621373,
        -0.0439192339348354905263921515528654458042,
        -0.0398459085471996706586162402473026835046,
        -0.0357891078515852792753420982122404025613,
        -0.0317486983145803011569962827485256299276,
        -0.0277245480148548604671395114515163869272,
        -0.0237165266173160421183468505286730579517,
        -0.0197245053477785891192717326571593033246,
        -0.015748356968139168607549511460828269521,
        -0.0117879557520422404691605618900871263399,
        -0.0078431774610258928731840424909435816546,
        -0.00391389932113632909231778364357266484272,
        0.0,
        0.00389864041565732301393734309584290701073,
        0.00778214044205494894746290006113676367813,
        0.01165061721997527413559144280921434893315,
        0.0155041865359652541508540460424468358779,
        0.01934296284313093463590553454155047018545,
        0.0231670592815343782287991609622899165794,
        0.0269765876982020757480692925396595457815,
        0.0307716586667536883710282075967721640917,
        0.0345523815066597334073715005898328652816,
        0.038318864302136599193755325123797290346,
        0.042071213920687054375203805926962379448,
        0.045809536031294203166679267614663342114,
        0.049533935122276630882096208829824573267,
        0.0532445145188122828658701937865287769396,
        0.0569413764001384247590131015404494943015,
        0.0606246218164348425806061320404202632862,
        0.0642943507053972572162284502656114944857,
        0.0679506619085077493945652777726294140346,
        0.071593653187008817925605272752092034269,
        0.075223421237587525698605339983662414637,
        0.078840061707776024531540577859198294559,
        0.082443669211074591268160068668307805914,
        0.086034337341803153381797826721996075141,
        0.0896121586896871326199514693784845287854,
        0.093177224854183289768781353027759396216,
        0.096729626458551112295571056487463437015,
        0.1002694531636751493081301751297276601964,
        0.1037967936816435648260618037639746883066,
        0.1073117357890880506671750303711543368066,
        0.1108143663402901141948061693232119280986,
        0.1143047712800586336342591448151747734094,
        0.1177830356563834545387941094705217050686,
        0.1212492436328696851612122640808405265723,
        0.1247034785009572358634065153808632684918,
        0.128145822691930038174109886961074873852,
        0.1315763577887192725887161286894831624516,
        0.134995164537504830601983291147085645626,
        0.138402322859119135685325873601649187393,
        0.1417979118602573498789527352804727189846,
        0.1451820098444978972819350637405643235226,
        0.1485546943231371429098223170672938691604,
        0.151916042025841975071803424896884511328,
        0.1552661289111239515223833017101021786436,
        0.1586050301766385840933711746258415752456,
        0.161932820269313253240338285123614220592,
        0.165249572895307162875611449277240313729,
        0.1685553610298066669415865321701023169345,
        0.171850256926659222340098946055147264935,
        0.1751343321278491480142914649863898412374,
        0.1784076574728182971194002415109419683545,
        0.181670303107634678260605595617079739242,
        0.184922338494011992663903592659249621006,
        0.1881638324181829868259905803105539806714,
        0.191394852999629454609298807561308873447,
        0.194615467699671658858138593767269731516,
        0.1978257433299198803625720711969614690756,
        0.201025746060590741340908337591797808969,
        0.204215541428690891503820386196239272214,
        0.2073951943460705871587455788490062338536,
        0.210564769107349637669552812732351513721,
        0.2137243293977181388619051976331987647734,
        0.216873938300614359619089525744347498479,
        0.220013658305282095907358638661628360712,
        0.2231435513142097557662950903098345033745,
        0.226263678650453389361787082280390161607,
        0.229374101064845829991480725046139871551,
        0.232474878743094064920705078095567528222,
        0.235566071312766909077588218941043410137,
        0.2386477378501750099171491363522813392526,
        0.241719936887145168144307515913513900104,
        0.244782726417690916434704717466314811104,
        0.247836163904581256780602765746524747999,
        0.25088030628580941658844644154994089393,
        0.253915209980963444137323297906606667466,
        0.256940930897500425446759867911224262093,
        0.259957524436926066972079494542311044577,
        0.26296504550088135182072917321108602859,
        0.265963548497137941339125926537543389269,
        0.268953087345503958932974357924497845489,
        0.271933715483641758831669494532999161983,
        0.274905485872799249167009582983018668293,
        0.277868451003456306186350032923401233082,
        0.280822662900887784639519758873134832073,
        0.28376817313064459834690122235025476666,
        0.286705032803954314653250930842073965668,
        0.289633292583042676878893055525668970004,
        0.292553002686377439978201258664126644308,
        0.295464212893835876386681906054964195182,
        0.298366972551797281464900430293496918012,
        0.301261330578161781012875538233755492657,
        0.304147335467296717015819874720446989991,
        0.30702503529491186207512454053537790169,
        0.309894477722864687861624550833227164546,
        0.31275571000389688838624655968831903216,
        0.315608778986303334901366180667483174144,
        0.318453731118534615810247213590599595595,
        0.321290612453734292057863145522557457887,
        0.324119468654211976090670760434987352183,
        0.326940344995853320592356894073809191681,
        0.329753286372467981814422811920789810952,
        0.332558337300076601412275626573419425269,
        0.335355541921137830257179579814166199074,
        0.338144944008716397710235913939267433111,
        0.340926586970593210305089199780356208443,
        0.34370051385331844468019789211029452987,
        0.346466767346208580918462188425772950712,
        0.349225389785288304181275421187371759687,
        0.35197642315717818465544745625943892599,
        0.354719909102929028355011218999317665826,
        0.357455888921803774226009490140904474434,
        0.360184403575007796281574967493016620926,
        0.362905493689368453137824345977489846141,
        0.365619199560964711319396875217046453067,
        0.368325561158707653048230154050398826898,
        0.371024618127872663911964910806824955394,
        0.373716409793584080821016832715823506644,
        0.376400975164253065997877633436251593315,
        0.379078352934969458390853345631019858882,
        0.38174858149084833985966626493567607862,
        0.384411698910332039734790062481290868519,
        0.387067742968448287898902502261817665695,
        0.38971675114002521337046360400352086705,
        0.392358760602863872479379611988215363485,
        0.39499380824086897810639403636498176831,
        0.397621930647138489104829072973405554918,
        0.40024316412701270692932510199513117008,
        0.402857544701083514655197565487057707577,
        0.405465108108164381978013115464349136572,
        0.408065889808221748430198682969084124381,
        0.410659924985268385934306203175822787661,
        0.41324724855021933092547601552548590025,
        0.415827895143710965613328892954902305356,
        0.418401899138883817510763261966760106515,
        0.42096929464412963612886716150679597245,
        0.423530115505803295718430478017910109426,
        0.426084395310900063124544879595476618897,
        0.428632167389698760206812276426639053152,
        0.43117346481837134085917247895559499848,
        0.433708320421559393435847903042186017095,
        0.436236766774918070349041323061121300663,
        0.438758836207627937745575058511446738878,
        0.441274560804875229489496441661301225362,
        0.443783972410300981171768440588146426918,
        0.446287102628419511532590180619669006749,
        0.448783982827006710512822115683937186274,
        0.451274644139458585144692383079012478686,
        0.453759117467120506644794794442263270651,
        0.456237433481587594380805538163929748437,
        0.458709622626976664843883309250877913511,
        0.461175715122170166367999925597855358603,
        0.463635740963032513092182277331163919118,
        0.466089729924599224558619247504769399859,
        0.468537711563239270375665237462973542708,
        0.470979715218791012546897856056359251373,
        0.473415770016672131372578393236978550606,
        0.475845904869963914265209586304381412175,
        0.478270148481470280383546145497464809096,
        0.480688529345751907676618455448011551209,
        0.48310107575113582273837458485214554795,
        0.485507815781700807801791077190788900579,
        0.487908777319238973246173184132656942487,
        0.490303988045193838150346159645746860531,
        0.492693475442575255695076950020077845328,
        0.495077266797851514597964584842833665358,
        0.497455389202818942250859256731684928918,
        0.499827869556449329821331415247044141512,
        0.502194734566715494273584171951812573586,
        0.504556010752395287058308531738174929982,
        0.506911724444854354113196312660089270034,
        0.509261901789807946804074919228323824878,
        0.51160656874906207851888487520338193135,
        0.51394575110223431680100608827421759311,
        0.51627947444845449617281928478756106467,
        0.518607764208045632152976996364798698556,
        0.520930645624185312409809834659637709188,
        0.52324814376454783651680722493487084164,
        0.525560283522927371382427602307131424923,
        0.527867089620842385113892217778300963557,
        0.530168586609121617841419630845212405063,
        0.532464798869471843873923723460142242606,
        0.534755750616027675477923292032637111077,
        0.537041465896883654566729244153832299024,
        0.539321968595608874655355158077341155752,
        0.54159728243274437157654230390043409897,
        0.543867430967283517663338989065998323965,
        0.546132437598135650382397209231209163864,
        0.548392325565573162748150286179863158565,
        0.550647117952662279259948179204913460093,
        0.552896837686677737580717902230624314327,
        0.55514150754050159271548035951590405017,
        0.557381150134006357049816540361233647898,
        0.559615787935422686270888500526826593487,
        0.561845443262691817915664819160697456814,
        0.564070138284802966071384290090190711817,
        0.566289895023115872590849979337124343595,
        0.568504735352668712078738764866962263577,
        0.5707146810034715448536245647415894503,
        0.572919753561785509092756726626261068625,
        0.575119974471387940421742546569273429365,
        0.577315365034823604318112061519496401506,
        0.579505946414642223855274409488070989814,
        0.58169173963462248252061075372537234071,
        0.583872765580982679097413356975291104927,
        0.586049045003578208904119436287324349516,
        0.588220598517086043034868221609113995052,
        0.590387446602176374641916708123598757576,
        0.59254960960667159874199020959329739696,
        0.594707107746692789514343546529205333192,
        0.59685996110779383658731192302565801002,
        0.59900818964608339938160002446165150206,
        0.601151813189334836191674317068856441547,
        0.603290851438084262340585186661310605647,
        0.6054253239667168894375677681414899356,
        0.607555250224541795501085152791125371894,
        0.609680649536855273481833501660588408785,
        0.611801541105992903529889766428814783686,
        0.613917944012370492196929119645563790777,
        0.616029877215514019647565928196700650293,
        0.618137359555078733872689126674816271683,
        0.620240409751857528851494632567246856773,
        0.62233904640877874159710264120869663505,
        0.62443328801189350104253874405467311991,
        0.626523152931352759778820859734204069282,
        0.628608659422374137744308205774183639946,
        0.6306898256261987050837261409313532241,
        0.63276666957103782954578646850357975849,
        0.634839209173010211969493840510489008123,
        0.63690746223706923162049442718119919119,
        0.63897144645792072137962398326473680873,
        0.64103117942093129105560133440539254671,
        0.643086678603027315392053859585132960477,
        0.645137961373584701665228496134731905937,
        0.647185044995309550122320631377863036675,
        0.64922794662510981889083996990531112227,
        0.651266683314958103396333353349672108398,
        0.653301272012745638758615881210873884572,
        0.65533172956312763209494967856962559648,
        0.657358072708360030141890023245936165513,
        0.659380318089127826115336413370955804038,
        0.661398482245365008260235838709650938148,
        0.66341258161706625109695030429080128179,
        0.665422632545090448950092610006660181147,
        0.667428651271956189947234166318980478403,
        0.669430653942629267298885270929503510123,
        0.67142865660530232331713904200189252584,
        0.67342267521216672029796038880101726475,
        0.67541272562017673108090414397019748722,
        0.677398823591806140809682609997348298556,
        0.67938098479579735014710062847376425181,
        0.681359224807903068948071559568089441735,
        0.683333559111620688164363148387750369654,
        0.68530400309891941654404807896723298642,
        0.687270572070960267497006884394346103924,
        0.689233281238808980324914337814603903233,
        0.691192145724141958859604629216309755938,
        0.693147180559945309417232121458176568075
      };
    return log_table[128 + (int)m] + 2.0 * atanh_z;
  }
}
