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
#include "orbit/core/coreLib.h"
#include "orbit/orbit/orbitLib.h"

// MARK: Earth Parameter
#define WGS84_A     6378137
#define WGS84_F     (1 / 298.2572235630)
#define WGS84_B     (WGS84_A * (1 - WGS84_F))
#define MIN_VISIBLE_ELEVATION_DEGREE        10
#define MIN_VISIBLE_ELEVATION_RADIAN        (radian(MIN_VISIBLE_ELEVATION_DEGREE))
#define MAX_FORECAST_DAY                    10

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


+ (void) getMoonRiseSetWithLongitude: (double) longitude latitude: (double) latitude forTime: (NSDate *) time completion:(nullable void (^)(NSDate *, NSDate *, NSDate *, NSDate *))handler {
    
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
    //--- 結果表示.
    
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
    
    handler(sr,ss,mr,ms);
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

+ (cJulian)julianDateFromDate:(NSDate *)time {
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    return cJulian((int)[compo year], (int)[compo month], (int)[compo day], (int)[compo hour], (int)[compo minute], 0);
}

+ (astro::AstroTime)astroDateFromDate:(NSDate *)time {
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    return astro::AstroTime(astro::Jday((int)[compo year],(int)[compo month],(int)[compo day]),(int)([compo hour] * 3600 + [compo minute] * 60));}

+ (SatelliteRiseSet *)getRiseSetForSatelliteWithTLE:(SatelliteTLE *)tle longitude: (double)longitude latitude: (double) latitude forTime: (NSDate *) time {
    using namespace std;
    /* Construct the TLE */
    string line0 = [tle.line0 UTF8String];
    string line1 = [tle.line1 UTF8String];
    string line2 = [tle.line2 UTF8String];
    cTle ctle(line0, line1, line2);
    /* Construct the Satellite */
    cSatellite sat(ctle);
    /* Construct the Julian Date */
    cJulian julian = [self julianDateFromDate:time];
    /* Setup Sun observer */
    astro::AstroCoordinate acoord;
    astro::Planets pl;
    astro::Degree lt(int(latitude),(latitude - int(latitude)) * 60, ((latitude - int(latitude)) * 60.0 - (int)((latitude - int(latitude)) * 60.0)) * 60);
    astro::Degree lg(int(longitude),(longitude - int(longitude)) * 60, ((longitude - int(longitude)) * 60.0 - (int)((longitude - int(longitude)) * 60.0)) * 60);
    acoord.setPosition(lg, lt);
    acoord.setLocation(lg, lt, 0);
    astro::AstroTime astroTime = [self astroDateFromDate:time];
    /* Current ECI */
    cEciTime eci = sat.PositionEci(julian);
    double A0, E0, R0;
    eci2aer(eci.Position().m_x * 1000, eci.Position().m_y * 1000, eci.Position().m_z * 1000, julian.ToGmst(), latitude, longitude, 0, A0, E0, R0);
    SatellitePosition *current = [[SatellitePosition alloc] initWithTime:time azimuth:A0 elevation:E0 range:R0];
    SatellitePosition *rise = nil;
    SatellitePosition *set = nil;
    SatellitePosition *peak = nil;
    double E1 = E0;
    double A1 = A0;
    double R1 = R0;
    for (int i = 1; i < MAX_FORECAST_DAY * 24 * 60; i++) {
        /* +1 Min */
        julian.AddMin(1);
        astroTime.addSec(60);
        cEciTime eci = sat.PositionEci(julian);
        double A, E, R;
        eci2aer(eci.Position().m_x * 1000, eci.Position().m_y * 1000, eci.Position().m_z * 1000, julian.ToGmst(), latitude, longitude, 0, A, E, R);
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
                acoord.setTime(astroTime);
                acoord.beginConvert();
                pl.calc(acoord);
                astro::Vec3 sun  = pl.vecQ(astro::Planets::SUN);
                acoord.conv_q2tq(sun);
                acoord.conv_q2h(sun);
                astro::Degree az, alt;
                sun.getLtLg(alt, az);
                if (alt.degree() < -6 && alt.degree() > -20) {
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
