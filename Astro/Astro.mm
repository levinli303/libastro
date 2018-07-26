//
//  Astro.m
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import "Astro.h"
#include "degree.h"
#include "acoord.h"
#include "planets.h"
#include <vector>
#include "ephem/astro.h"

#undef lat

// MARK: Earth Parameter
#define WGS84_A     6378137
#define WGS84_F     (1 / 298.2572235630)
#define WGS84_B     (WGS84_A * (1 - WGS84_F))
#define MIN_VISIBLE_ELEVATION_DEGREE        10
#define MIN_VISIBLE_ELEVATION_RADIAN        (radian(MIN_VISIBLE_ELEVATION_DEGREE))
#define MAX_FORECAST_DAY                    5

// MARK: Geo Conversion
double radian(const double &degree)
{
    return degree / 180.0 * M_PI;
}

void _uvw2enu(double u, double v, double w, double lat, double lon, double &E, double &N, double &U)
{
    lon = radian(lon);
    lat = radian(lat);
    double t = cos(lon) * u + sin(lon) * v;
    E = -sin(lon) * u + cos(lon) * v;
    U = cos(lat) * t + sin(lat) * w;
    N = -sin(lat) * t + cos(lat) * w;
}

double get_radius_normal(double lat)
{
    return pow(WGS84_A, 2) / sqrt(pow(WGS84_A, 2) * pow(cos(lat), 2) + pow(WGS84_B, 2) * pow(sin(lat), 2));
}

void geodetic2ecef(double lat, double lon, double h, double &X, double &Y, double &Z)
{
    lon = radian(lon);
    lat = radian(lat);
    double N = get_radius_normal(lat);
    X = (N + h) * cos(lat) * cos(lon);
    Y = (N + h) * cos(lat) * sin(lon);
    Z = (N * pow(WGS84_B / WGS84_A, 2) + h) * sin(lat);
}

void ecef2enu(double x, double y, double z, double lat, double lon, double h, double &E, double &N, double &U)
{
    double x0, y0, z0;
    geodetic2ecef(lat, lon, h, x0, y0, z0);
    _uvw2enu(x - x0, y - y0, z - z0, lat, lon, E, N, U);
}

void enu2aer(double e, double n, double u, double &A, double &E, double &R)
{
    double r = hypot(e, n);
    const double two_pi = M_PI * 2;
    R = hypot(u, r);
    E = atan2(u, r);
    double a = atan2(e, n);
    A = a - two_pi * floor(a / two_pi);
}

void ecef2aer(double x, double y, double z, double lat, double lon, double h, double &A, double &E, double &R)
{
    double xEast, yNorth, zUp;
    ecef2enu(x, y, z, lat, lon, h, xEast, yNorth, zUp);
    enu2aer(xEast, yNorth, zUp, A, E, R);
}

void eci2ecef(double x, double y, double z, double t, double &X, double &Y, double &Z)
{
    double cos_s = cos(t);
    double sin_s = sin(t);

    X =   x * cos_s  + y * sin_s;
    Y =  -x * sin_s  + y * cos_s;
    Z =   z;
}

void eci2aer(double x, double y, double z, double t, double lat, double lon, double h, double &A, double &E, double &R)
{
    double X, Y, Z;
    eci2ecef(x, y, z, t, X, Y, Z);
    ecef2aer(X, Y, Z, lat, lon, h, A, E, R);
}

@implementation AstroPosition
- (instancetype)init
{
    self = [super init];
    if (self) {
        self.azimuth = 0;
        self.elevation = 0;
        self.time = [NSDate date];
    }
    return self;
}

- (id)copyWithZone:(NSZone *)zone {
    AstroPosition *pos = [AstroPosition allocWithZone:zone];
    if (pos) {
        [self copyPropertyTo:pos];
    }
    return pos;
}

- (void)copyPropertyTo:(AstroPosition *)copy {
    copy.azimuth = self.azimuth;
    copy.elevation = self.elevation;
    copy.time = [self.time copy];
}
@end

@implementation AstroRiset
- (instancetype)initWithRise:(AstroPosition *)rise peak:(AstroPosition *)peak set:(AstroPosition *)set {
    self = [super init];
    if (self) {
        self.rise = rise;
        self.peak = peak;
        self.set = set;
    }
    return self;
}
@end

@interface SatelliteTLE ()
@property (nonatomic, copy) NSString *line0;
@property (nonatomic, copy) NSString *line1;
@property (nonatomic, copy) NSString *line2;
@end

@implementation SatelliteTLE : NSObject
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 Line2: (NSString *)line2 {
    self = [super init];
    if (self) {
        self.line0 = [line0 copy];
        self.line1 = [line1 copy];
        self.line2 = [line2 copy];
    }
    return self;
}
@end

@implementation SatellitePosition
- (instancetype)initWithTime: (const NSDate *)time azimuth: (double)azimuth elevation: (double)elevation range: (double)range {
    self = [super init];
    if (self) {
        self.time = [time copy];
        self.azimuth = azimuth;
        self.elevation = elevation;
        self.range = range;
    }
    return self;
}
@end

@implementation SatelliteRiseSet
- (instancetype)initWithName: (const NSString *)name current: (SatellitePosition *)current rise: (SatellitePosition *)rise peak: (SatellitePosition *)peak set: (SatellitePosition *)set {
    self = [super init];
    if (self) {
        self.name = [name copy];
        self.current = current;
        self.rise = rise;
        self.peak = peak;
        self.set = set;
    }
    return self;
}
@end

@implementation AstroRiseSet
- (instancetype)initWithName: (const NSString *)name rise: (NSDate *)rise set: (NSDate *)set {
    self = [super init];
    if (self) {
        self.name = [name copy];
        self.rise = [rise copy];
        self.set = [set copy];
    }
    return self;
}
@end

@implementation LunarPhase
- (instancetype)init {
    self = [super init];
    if (self) {
        self.name = @"";
        self.nextFull = [NSDate date];
        self.nextNew = [NSDate date];
        self.phase = 0;
    }
    return self;
}
@end

@implementation Astro

int vecFromPlanet(AstroPlanet plantet)
{
    switch (plantet) {
        case AstroPlanetSun:
            return astro::Planets::SUN;
        case AstroPlanetMoon:
            return astro::Planets::MOON;
        case AstroPlanetMercury:
            return astro::Planets::MERCURY;
        case AstroPlanetVenus:
            return astro::Planets::VENUS;
        case AstroPlanetMars:
            return astro::Planets::MARS;
        case AstroPlanetJupiter:
            return astro::Planets::JUPITER;
        case AstroPlanetSaturn:
            return astro::Planets::SATURN;
        case AstroPlanetUranus:
            return astro::Planets::URANUS;
        case AstroPlanetNeptune:
            return astro::Planets::NEPTUNE;
        case AstroPlanetPluto:
            return astro::Planets::PLUTO;
    }
}

const NSString *nameFromPlanet(int planet)
{
    switch (planet) {
        case AstroPlanetSun:
            return @"Sun";
        case AstroPlanetMoon:
            return @"Moon";
        case AstroPlanetMercury:
            return @"Mercury";
        case AstroPlanetVenus:
            return @"Venus";
        case AstroPlanetMars:
            return @"Mars";
        case AstroPlanetJupiter:
            return @"Jupiter";
        case AstroPlanetSaturn:
            return @"Saturn";
        case AstroPlanetUranus:
            return @"Uranus";
        case AstroPlanetNeptune:
            return @"Neptune";
        case AstroPlanetPluto:
            return @"Pluto";
    }
    return @"Unknown";
}

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

+ (NSDate *) dateFromAstroTime: (astro::AstroTime &) atime {
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDateComponents *components = [[NSDateComponents alloc] init];
    
    int y, m, d;
    double utc;
    atime.get(y, m, d, utc);
    
    [components setDay: d];
    [components setMonth: m];
    [components setYear: y];
    [components setHour: 0];
    [components setMinute: 0];
    [components setSecond: 0];
    [components setTimeZone: [NSTimeZone timeZoneForSecondsFromGMT:0]];
    
    NSDate *date = [calendar dateFromComponents:components];
    return [date dateByAddingTimeInterval:utc];
}

+ (void)getRiseSetWithLongitude:(double) longitude latitude: (double) latitude forTime: (NSDate *) time completion:(void (^)(AstroRiset *sun, AstroRiset *moon))handler {

    // Position info
    astro::Degree lt(latitude * 3600);
    astro::Degree lg(longitude * 3600);
    // TODO: add height
    double sea = 0;

    astro::AstroCoordinate acoord;
    astro::Planets pl;
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, sea);

    // Time info
    astro::AstroTime astroTime = [self astroDateFromDate:time];
    // Starts from previous day
    astroTime.addDay(-1);
    acoord.setTime(astroTime);
    acoord.beginConvert();
    pl.calc(acoord);

    // Sun and moon
    astro::Vec3 sun  = pl.vecQ(astro::Planets::SUN);
    astro::Vec3 moon = pl.vecQ(astro::Planets::MOON);
    acoord.conv_q2tq(sun);
    acoord.conv_q2tq(moon);
    acoord.conv_q2h(sun);
    acoord.conv_q2h(moon);
    acoord.addRefraction(sun);
    acoord.addRefraction(moon);

    astro::AstroTime t = acoord.getTime();
    // Calculate up to 2 days
    const double jd_end = t.jd() + 2;
    const double sun_rz  = sin(astro::dms2rad(0,0,960));
    const double min30_z = sin(astro::hms2rad(0,30,0));
    const double min3_z  = sin(astro::hms2rad(0,3,0));
    const double sec15_z = sin(astro::hms2rad(0,0,15));
    int step = -1;    // Backward 1s to calculate

    AstroPosition *sunpeak = nil;
    AstroPosition *moonpeak = nil;
    AstroPosition *sunrise = nil;
    AstroPosition *moonrise = nil;

    AstroRiset *sunriset = nil;
    AstroRiset *moonriset = nil;
    t.addSec(step);
    NSDate *currentTime = [self dateFromAstroTime:t];
    for (; t.jd() < jd_end; t.addSec(step)) {
        // Save last data for comparation
        const astro::Vec3 sun0 = sun;
        const astro::Vec3 moon0 = moon;
        // Calculate current position
        acoord.setTime(t);
        acoord.beginConvert();
        pl.calc(acoord);
        sun  = pl.vecQ(astro::Planets::SUN);
        moon = pl.vecQ(astro::Planets::MOON);
        acoord.conv_q2tq(sun);
        acoord.conv_q2tq(moon);
        acoord.conv_q2h(sun);
        acoord.conv_q2h(moon);
        acoord.addRefraction(sun);
        acoord.addRefraction(moon);
        sun.z += sun_rz;
        moon.z += sun_rz;
        if (step > 0) {
            astro::Degree az0, el0, az, el;
            double azDeg, azDeg0, elDeg, elDeg0;

            // Sun
            if (sunriset == nil) {
                sun.getLtLg(el, az);
                sun0.getLtLg(el0, az0);
                // South 0, West 90, North 180, East 270
                az.setNeg();
                az.mod360();
                az0.setNeg();
                az0.mod360();
                azDeg = az.degree();
                azDeg0 = az0.degree();
                elDeg = el.degree();
                elDeg0 = el0.degree();

                if (elDeg0 < 0 && elDeg >= 0) {
                    // Sunrise
                    sunrise = [AstroPosition new];
                    sunrise.azimuth = azDeg;
                    sunrise.elevation = elDeg;
                    sunrise.time = currentTime;
                    sunpeak = [sunrise copy];
                } else if (elDeg0 >=0 && elDeg < 0) {
                    // Sunset
                    if ([currentTime timeIntervalSinceDate:time] < 0) {
                        // Already happened, disregard & clear
                        sunrise = nil;
                        sunpeak = nil;
                    } else if (sunpeak != nil && sunrise != nil) {
                        // Disregard if there is no rise/peak
                        AstroPosition *sunset = [AstroPosition new];
                        sunset.azimuth = azDeg;
                        sunset.elevation = elDeg;
                        sunset.time = currentTime;

                        sunriset = [[AstroRiset alloc] initWithRise:sunrise peak:sunpeak set:sunset];

                        // Clear rise & peak
                        sunrise = nil;
                        sunpeak = nil;
                    }
                } else if (elDeg > elDeg0 && sunpeak != nil && sunrise != nil) {
                    sunpeak.azimuth = azDeg;
                    sunpeak.elevation = elDeg;
                    sunpeak.time = currentTime;
                }
            }

            if (moonriset == nil) {
                // Moon
                moon.getLtLg(el, az);
                moon0.getLtLg(el0, az0);
                // South 0, West 90, North 180, East 270
                az.setNeg();
                az.mod360();
                az0.setNeg();
                az0.mod360();
                azDeg = az.degree();
                azDeg0 = az0.degree();
                elDeg = el.degree();
                elDeg0 = el0.degree();

                if (elDeg0 < 0 && elDeg >= 0) {
                    // Moonrise
                    moonrise = [AstroPosition new];
                    moonrise.azimuth = azDeg;
                    moonrise.elevation = elDeg;
                    moonrise.time = currentTime;
                    moonpeak = [moonrise copy];
                } else if (elDeg0 >=0 && elDeg < 0) {
                    // Moonset
                    if ([currentTime timeIntervalSinceDate:time] < 0) {
                        // Already happened, disregard & clear
                        moonrise = nil;
                        moonpeak = nil;
                    } else if (moonpeak != nil && moonrise != nil) {
                        // Disregard if there is no rise/peak
                        AstroPosition *moonset = [AstroPosition new];
                        moonset.azimuth = azDeg;
                        moonset.elevation = elDeg;
                        moonset.time = currentTime;

                        moonriset = [[AstroRiset alloc] initWithRise:moonrise peak:moonpeak set:moonset];

                        // Clear rise & peak
                        moonrise = nil;
                        moonpeak = nil;
                    }
                } else if (elDeg > elDeg0 && moonpeak != nil && moonrise != nil) {
                    moonpeak.azimuth = azDeg;
                    moonpeak.elevation = elDeg;
                    moonpeak.time = currentTime;
                }
            }
        }

        if (sunriset != nil && moonriset != nil) {
            break;
        }

        // Calculate the step size
        double z = min(fabs(sun.z), fabs(moon.z));
        double y = min(fabs(sun.y), fabs(moon.y));
        z = min(z, y);
        if (z >= min30_z)
            step = 20*60;
        else if (z >= min3_z)
            step = 2*60;
        else if (z >= sec15_z)
            step = 10;
        else
            step = 1;

        // Recalculate current time
        currentTime = [currentTime dateByAddingTimeInterval:step];
    }

    if (handler != nil) {
        handler(sunriset, moonriset);
    }
}

+ (void) getMoonRiseSetWithLongitude: (double) longitude latitude: (double) latitude forTime: (NSDate *) time completion:(void (^)(NSDate *, NSDate *, NSDate *, NSDate *, LunarPhase *))handler {
    
    astro::Degree lt(int(latitude),(latitude - int(latitude)) * 60, ((latitude - int(latitude)) * 60.0 - (int)((latitude - int(latitude)) * 60.0)) * 60);
    astro::Degree lg(int(longitude),(longitude - int(longitude)) * 60, ((longitude - int(longitude)) * 60.0 - (int)((longitude - int(longitude)) * 60.0)) * 60);
    double sea = 0;
    
    astro::AstroCoordinate acoord;
    astro::Planets pl;
    
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, sea);
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    astro::AstroTime astroTime(astro::Jday((int)[compo year],(int)[compo month],(int)[compo day]),(int)([compo hour] * 3600 + [compo minute] * 60 + [compo second]));
    astroTime.addDay(-1);
    acoord.setTime(astroTime);
    acoord.beginConvert();
    pl.calc(acoord);
    astro::Vec3 sun  = pl.vecQ(astro::Planets::SUN);
    astro::Vec3 moon = pl.vecQ(astro::Planets::MOON);
    
    NSMutableArray *moons = [NSMutableArray array];
    NSMutableArray *suns = [NSMutableArray array];
    
    astro::AstroTime t = acoord.getTime();
    const double jd_end = t.jd() + 2;
    const double sun_rz  = sin(astro::dms2rad(0,0,960));	// 太陽視半径による出没補正. 視半径は 960" で決め打ち.
    const double min30_z = sin(astro::hms2rad(0,30,0));	// 時角30分の高度のz座標値.
    const double min3_z  = sin(astro::hms2rad(0,3,0));	// 時角1分の高度のz座標値.
    const double sec15_z = sin(astro::hms2rad(0,0,15));	// 時角15秒の高度のz座標値.
    int step = -1;	// 初回は指定時刻の1秒前の高度を計算する.
    for (t.addSec(step); t.jd() < jd_end; t.addSec(step)) {
        // 前回時刻の高度を保存する. ただし、初回はこの値を使ってはいけない.
        const astro::Vec3 sun0 = sun;
        const astro::Vec3 moon0 = moon;
        // 今回時刻の高度を計算する.
        acoord.setTime(t);
        acoord.beginConvert();
        pl.calc(acoord);
        sun  = pl.vecQ(astro::Planets::SUN);
        moon = pl.vecQ(astro::Planets::MOON);
        acoord.conv_q2tq(sun);
        acoord.conv_q2tq(moon);
        acoord.conv_q2h(sun);
        acoord.conv_q2h(moon);
        // 大気差補正は常時実施する.
        acoord.addRefraction(sun);
        acoord.addRefraction(moon);
        // 太陽視半径分を高度補正する.
        sun.z += sun_rz;
        moon.z += sun_rz;
        if (step > 0) {
            // 前回時刻の高度と比較し、境界値を跨いだ時刻を出没時刻として表示する.
            if (sun0.z < 0 && sun.z >= 0)
            {
                [suns addObject:@{@"rs":@"Rise",@"time":[self dateFromAstroTime:t]}];
            }
            if (sun0.z >= 0 && sun.z < 0)
            {
                [suns addObject:@{@"rs":@"Set",@"time":[self dateFromAstroTime:t]}];
            }
            if (moon0.z < 0 && moon.z >= 0)
            {
                [moons addObject:@{@"rs":@"Rise",@"time":[self dateFromAstroTime:t]}];
            }
            if (moon0.z >= 0 && moon.z < 0)
            {
                [moons addObject:@{@"rs":@"Set",@"time":[self dateFromAstroTime:t]}];
            }
        }
        double z = min(fabs(sun.z), fabs(moon.z));
        double y = min(fabs(sun.y), fabs(moon.y));
        z = min(z, y);	// 地平線通過、子午線通過付近の最小座標値を求める.
        if (z >= min30_z)
            step = 20*60; // 高度が±時角30分以上なら20分単位で時刻を進める.
        else if (z >= min3_z)
            step = 2*60; // 高度が±時角3分以上なら2分単位で時刻を進める.
        else if (z >= sec15_z)
            step = 10; // 高度が±時角15秒以上なら10秒単位で時刻を進める.
        else
            step = 1; // 1秒単位で時刻を進める.
    }
    
    NSDate *mr = nil;
    NSDate *ms = nil;
    NSDate *sr = nil;
    NSDate *ss = nil;
    
    if ([suns count] > 1) {
        for (int i = 0; i < [suns count] - 1; i++) {
            NSDate *curr = suns[i][@"time"];
            NSDate *next = suns[i + 1][@"time"];
            NSString *currType = suns[i][@"rs"];
            NSString *nextType = suns[i + 1][@"rs"];
            if ([currType isEqualToString:@"Rise"] && [nextType isEqualToString:@"Set"]) {
                if ([curr timeIntervalSinceDate:time] < 0 && [next timeIntervalSinceDate:time] > 0) {
                    sr = curr;
                    ss = next;
                    break;
                }
                if ([curr timeIntervalSinceDate:time] >= 0) {
                    sr = curr;
                    ss = next;
                    break;
                }
            }
        }
    }
    
    if ([moons count] > 1) {
        for (int i = 0; i < [moons count] - 1; i++) {
            NSDate *curr = moons[i][@"time"];
            NSDate *next = moons[i + 1][@"time"];
            NSString *currType = moons[i][@"rs"];
            NSString *nextType = moons[i + 1][@"rs"];
            if ([currType isEqualToString:@"Rise"] && [nextType isEqualToString:@"Set"]) {
                if ([curr timeIntervalSinceDate:time] < 0 && [next timeIntervalSinceDate:time] > 0) {
                    mr = curr;
                    ms = next;
                    break;
                }
                if ([curr timeIntervalSinceDate:time] >= 0) {
                    mr = curr;
                    ms = next;
                    break;
                }
            }
        }
    }
    if (handler != nil) {
        handler(sr,ss,mr,ms, [self getCurrentMoonPhase]);
    }
}

double fmod2(double m1, double m2)
{
    return m1 - floor(m1 / m2) * m2;
}

double calc_phase(double x, double antitarget)
{
    Obj moonObj;
    memset(&moonObj, 0, sizeof(Obj));
    moonObj.pl.plo_code = MOON;
    moonObj.any.co_type = PLANET;
    Obj sunObj;
    memset(&sunObj, 0, sizeof(Obj));
    sunObj.pl.plo_code = SUN;
    sunObj.any.co_type = PLANET;
    Now now;
    memset(&now, 0, sizeof(Now));
    now.n_mjd = x;
    now.n_pressure = 1010;
    obj_cir(&now, &sunObj);
    obj_cir(&now, &moonObj);
    double slon, slat, mlon, mlat;
    eq_ecl(now.n_mjd, sunObj.pl.co_gaera, sunObj.pl.co_gaedec, &slat, &slon);
    eq_ecl(now.n_mjd, moonObj.pl.co_gaera, moonObj.pl.co_gaedec, &mlat, &mlon);
    double res = fmod2(mlon - slon - antitarget, 2 * M_PI) - M_PI;
    return res;
}

+(NSDate *)findMoonPhase: (NSDate *)d motion:(double) motion target:(double)target {
    double antitarget = target + M_PI;
    double time = [self modifiedJulianDateFromDate:d];
    double res = calc_phase(time, antitarget);
    double angle_to_cover = fmod2(-res, motion);
    double dd = time + 29.53 * angle_to_cover / (2 * M_PI);
    double hour = 1.0 / 24;
    double x0 = dd;
    double x1 = dd + hour;
    double f0 = calc_phase(x0, antitarget);
    double f1 = calc_phase(x1, antitarget);
    while (fabs(x1 - x0) > 1.0 / 24 / 60 && f1 != f0) {
        double x2 = x0;
        x0 = x1;
        x1 = x1 + (x1 - x2) / (f0 / f1 - 1);
        f0 = f1;
        f1 = calc_phase(x1, antitarget);
    }
    return [d dateByAddingTimeInterval:(x1 - time) * 86400.0];
}

+ (LunarPhase *)getCurrentMoonPhase {
    // Calculate lunar phase
    LunarPhase *p = [[LunarPhase alloc] init];
    NSDate *time = [NSDate date];
    NSDate *prevNew = [self findMoonPhase:time motion:M_PI * -2 target:0];
    p.nextNew = [self findMoonPhase:time motion:M_PI * 2 target:0];
    p.nextFull = [self findMoonPhase:time motion:M_PI * 2 target:M_PI];
    double phase = [time timeIntervalSinceDate:prevNew] / [p.nextNew timeIntervalSinceDate:prevNew];
    p.phase = phase;

    if (phase <= 0.01 || phase >= 0.99) {
        p.name = @"New Moon";
    } else if (phase < 0.24) {
        p.name = @"Waxing Crescent";
    } else if (phase <= 0.26) {
        p.name = @"First Quarter";
    } else if (phase < 0.49) {
        p.name = @"Waxing Gibbous";
    } else if (phase <= 0.51) {
        p.name = @"Full Moon";
    } else if (phase < 0.74) {
        p.name = @"Waning Crescent";
    } else if (phase < 0.76) {
        p.name = @"Last Quarter";
    } else if (phase < 0.99) {
        p.name = @"Waning Gibbous";
    }
    return p;
}

+ (NSArray *)getRiseSetForAllSolarSystemObjectsInLongitude:(double) longitude latitude: (double) latitude forTime: (NSDate *) time {
    astro::Degree lt(int(latitude),(latitude - int(latitude)) * 60, ((latitude - int(latitude)) * 60.0 - (int)((latitude - int(latitude)) * 60.0)) * 60);
    astro::Degree lg(int(longitude),(longitude - int(longitude)) * 60, ((longitude - int(longitude)) * 60.0 - (int)((longitude - int(longitude)) * 60.0)) * 60);
    double sea = 0;
    
    astro::AstroCoordinate acoord;
    astro::Planets pl;
    
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, sea);
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    astro::AstroTime astroTime(astro::Jday((int)[compo year], (int)[compo month],(int)[compo day]),
                               (int)([compo hour] * 3600 + [compo minute] * 60 + [compo second]));
    // calculation start the day before it
    astroTime.addDay(-1);
    acoord.setTime(astroTime);
    acoord.beginConvert();
    pl.calc(acoord);
    
    // record the first calculated plvs
    std::vector<astro::Vec3> plvs;
    NSMutableArray *pls = [NSMutableArray array];
    for (int i = astro::Planets::SUN; i <= astro::Planets::PLUTO; i++)
    {
        astro::Vec3 vec = pl.vecQ(i);
        acoord.conv_q2tq(vec);
        acoord.conv_q2h(vec);
        // 大気差補正は常時実施する.
        acoord.addRefraction(vec);
        if (i == astro::Planets::SUN || i == astro::Planets::MOON)
        {
            vec.z += sin(astro::dms2rad(0,0,960)); // 太陽視半径分を高度補正する.
        }
        plvs.push_back(pl.vecQ(i));
        [pls addObject:[NSMutableArray array]];
    }
    astro::AstroTime t = acoord.getTime();
    const double jd_end = t.jd() + 2;
    const int step = 60;
    for (t.addSec(step); t.jd() < jd_end; t.addSec(step)) {
        // 前回時刻の高度を保存する. ただし、初回はこの値を使ってはいけない.
        std::vector<astro::Vec3> newPlvs;
        // 今回時刻の高度を計算する.
        acoord.setTime(t);
        acoord.beginConvert();
        pl.calc(acoord);
        for (int i = astro::Planets::SUN; i <= astro::Planets::PLUTO; i++)
        {
            astro::Vec3 vec = pl.vecQ(i);
            acoord.conv_q2tq(vec);
            acoord.conv_q2h(vec);
            // 大気差補正は常時実施する.
            acoord.addRefraction(vec);
            if (i == astro::Planets::SUN || i == astro::Planets::MOON)
            {
                vec.z += sin(astro::dms2rad(0,0,960)); // 太陽視半径分を高度補正する.
            }
            if (vec.z >= 0 && plvs[i].z < 0)
            {
                [pls[i] addObject:@{@"rs":@"Rise",@"time":[self dateFromAstroTime:t]}];
            }
            if (vec.z < 0 && plvs[i].z >= 0)
            {
                [pls[i] addObject:@{@"rs":@"Set",@"time":[self dateFromAstroTime:t]}];
            }
            newPlvs.push_back(vec);
        }
        plvs = newPlvs;
    }

    NSMutableArray *resultArray = [NSMutableArray array];
    for (int i = 0; i < [pls count]; i++) {
        NSDate *r = nil;
        NSDate *s = nil;
        for (int j = 0; j < [pls[i] count] - 1; j++) {
            NSDate *curr = pls[i][j][@"time"];
            NSDate *next = pls[i][j + 1][@"time"];
            NSString *currType = pls[i][j][@"rs"];
            NSString *nextType = pls[i][j + 1][@"rs"];
            if ([currType isEqualToString:@"Rise"] && [nextType isEqualToString:@"Set"]) {
                if ([next timeIntervalSinceDate:time] > 0) {
                    r = curr;
                    s = next;
                    break;
                }
            }
        }
        [resultArray addObject:[[AstroRiseSet alloc] initWithName:nameFromPlanet((AstroPlanet)i) rise:r set:s]];
    }
    return resultArray;
}

+ (astro::AstroTime)astroDateFromDate:(NSDate *)time {
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    return astro::AstroTime(astro::Jday((int)[compo year],(int)[compo month],(int)[compo day]),(int)([compo hour] * 3600 + [compo minute] * 60));}

+ (double)modifiedJulianDateFromDate:(NSDate *)time {
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    double x;
    cal_mjd((int)[compo month], [compo day] + ([compo hour] * 3600 + [compo minute] * 60) / 86400.0, (int)[compo year], &x);
    return x;
}

+ (SatelliteRiseSet *)getRiseSetForSatelliteWithTLE:(SatelliteTLE *)tle longitude: (double)longitude latitude: (double) latitude forTime: (NSDate *) time {
    using namespace std;
    /* Construct the TLE */
    string line0 = [tle.line0 UTF8String];
    string line1 = [tle.line1 UTF8String];
    string line2 = [tle.line2 UTF8String];
    Obj satillite_b;
    Obj satillite;
    /* Construct the Satellite */
    db_tle((char *)[tle.line0 UTF8String], (char *)[tle.line1 UTF8String], (char *)[tle.line2 UTF8String], &satillite_b);
    memcpy(&satillite, &satillite_b, sizeof(Obj));
    /* Construct the observer */
    Now now;
    memset(&now, 0, sizeof(Now));
    now.n_lng = radian(longitude);
    now.n_lat = radian(latitude);
    now.n_elev = 0;
    now.n_tz = 0;
    /* Construct the Julian Date */
    now.n_mjd = [self modifiedJulianDateFromDate:time];
    now.n_pressure = 1010;
    /* Current Position */
    obj_earthsat(&now, &satillite);
    double A0 = satillite.es.co_az;
    double E0 = satillite.es.co_alt;
    double R0 = satillite.es.ess_range;
    SatellitePosition *current = [[SatellitePosition alloc] initWithTime:time azimuth:A0 elevation:E0 range:R0];
    SatellitePosition *rise = nil;
    SatellitePosition *set = nil;
    SatellitePosition *peak = nil;
    double E1 = E0;
    double A1 = A0;
    double R1 = R0;
    for (int i = 1; i < MAX_FORECAST_DAY * 24 * 60; i++) {
        /* +1 Min */
        now.n_mjd += (60 / 86400.0);
        memcpy(&satillite, &satillite_b, sizeof(Obj));
        obj_earthsat(&now, &satillite);
        double A = satillite.es.co_az;
        double E = satillite.es.co_alt;
        double R = satillite.es.ess_range;
        if (rise == nil && E >= 0 && E1 < 0) {
            rise = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * i] azimuth:A elevation:E range:R];
            peak = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * i] azimuth:A elevation:E range:R];
            set = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * i] azimuth:A elevation:E range:R];
        } else if (rise != nil && E > E1) {
            peak = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * i] azimuth:A elevation:E range:R];
            set = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * i] azimuth:A elevation:E range:R];
        } else if (rise != nil && E < 0 && E1 >= 0) {
            set = [[SatellitePosition alloc] initWithTime:[time dateByAddingTimeInterval:60 * (i - 1)] azimuth:A1 elevation:E1 range:R1];
            if (peak.elevation < MIN_VISIBLE_ELEVATION_RADIAN) {
                /* too small retry */
                rise = nil;
                set = nil;
                peak = nil;
            } else {
                Now sunNow;
                memcpy(&sunNow, &now, sizeof(Now));
                sunNow.n_mjd = [self modifiedJulianDateFromDate:peak.time];
                Obj sunObj;
                sunObj.pl.plo_code = SUN;
                sunObj.any.co_type = PLANET;
                obj_cir(&sunNow, &sunObj);
                double alt_degree = sunObj.pl.co_alt / M_PI * 180;
                if (alt_degree < -6 && alt_degree > -30 && !satillite.es.ess_eclipsed) {
                    break;
                } else {
                    rise = nil;
                    set = nil;
                    peak = nil;
                }
            }
        }
        E1 = E;
        A1 = A;
        R1 = R;
    }
    return [[SatelliteRiseSet alloc] initWithName:tle.line0 current:current rise:rise peak:peak set:set];
}
@end
