//
//  Astro.m
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import "Astro.h"
#include "ephem/astro.h"

#undef lat

#define MIN_VISIBLE_ELEVATION_DEGREE        10
#define MIN_VISIBLE_ELEVATION_RADIAN        (radian(MIN_VISIBLE_ELEVATION_DEGREE))
#define MAX_FORECAST_DAY                    5

// MARK: Geo Conversion
double radian(const double &degree)
{
    return degree / 180.0 * M_PI;
}

NSDate *ModernDate(double d)
{
    int year, month;
    double dday;
    mjd_cal(d, &month, &dday, &year);
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [[NSDateComponents alloc] init];
    [compo setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
    [compo setYear:year];
    [compo setMonth:month];
    NSInteger day = round(dday);
    [compo setDay:day];
    double dhour = (dday - day) * 24;
    NSInteger hour = round(dhour);
    [compo setHour:hour];
    double dminute = (dhour - hour) * 60;
    NSInteger minute = round(dminute);
    [compo setMinute:minute];
    double dsecond = (dminute - minute) * 60;
    NSInteger second = round(dsecond);
    [compo setSecond:second];
    return [calendar dateFromComponents:compo];
}

double ModifiedJulianDate(NSDate *time)
{
    NSCalendar *calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
    NSDateComponents *compo = [calendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0] fromDate:time];
    double x;
    cal_mjd((int)[compo month], [compo day] + ([compo hour] * 3600 + [compo minute] * 60 + [compo second]) / 86400.0, (int)[compo year], &x);
    return x;
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

const NSString *NameFromLibastro(int index)
{
    switch (index) {
        case SUN:
            return @"Sun";
        case MOON:
            return @"Moon";
        case MERCURY:
            return @"Mercury";
        case VENUS:
            return @"Venus";
        case MARS:
            return @"Mars";
        case JUPITER:
            return @"Jupiter";
        case SATURN:
            return @"Saturn";
        case URANUS:
            return @"Uranus";
        case NEPTUNE:
            return @"Neptune";
        case PLUTO:
            return @"Pluto";
    }
    return @"Unknown";
}

AstroRiset *RisetForObserver(Now *now, int index)
{
    RiseSet rs;

    Obj *objects;
    getBuiltInObjs(&objects);

    riset_cir(now, &objects[index], 0, &rs);

    if (rs.rs_flags != 0) {
        return nil;
    }

    AstroPosition *rise = [AstroPosition new];
    rise.azimuth = rs.rs_riseaz;
    rise.elevation = 0;
    rise.time = ModernDate(rs.rs_risetm);

    AstroPosition *transit = [AstroPosition new];
    transit.azimuth = rs.rs_tranaz;
    transit.elevation = rs.rs_tranalt;
    transit.time = ModernDate(rs.rs_trantm);

    AstroPosition *set = [AstroPosition new];
    set.azimuth = rs.rs_setaz;
    set.elevation = 0;
    set.time = ModernDate(rs.rs_settm);

    AstroRiset *riset = [[AstroRiset alloc] initWithRise:rise peak:transit set:set];;
    riset.name = [NameFromLibastro(index) copy];

    return riset;
}

AstroRiset *ModifiedRisetForObserver(Now *now, int index)
{
    Now backup;
    memcpy(&backup, now, sizeof(Now));

    AstroRiset *today = RisetForObserver(&backup, index);

    if (today == nil) {
        return nil;
    }

    if ([today.peak.time timeIntervalSinceDate: today.rise.time] >= 0 && [today.set.time timeIntervalSinceDate: today.rise.time] >= 0) {
        return today;
    }

    BOOL go_back = [ModernDate(now->n_mjd) timeIntervalSinceDate:today.rise.time] < 0;
    if (go_back) {
        backup.n_mjd -= 1;

        AstroRiset *yesterday = RisetForObserver(&backup, index);
        if (yesterday == nil) {
            return nil;
        }

        if ([today.set.time timeIntervalSinceDate: today.rise.time] < 0) {
            today.rise = yesterday.rise;
            today.peak = yesterday.peak;
        } else if ([today.peak.time timeIntervalSinceDate: today.rise.time] < 0) {
            today.rise = yesterday.rise;
        }
        return today;
    }

    backup.n_mjd += 1;
    AstroRiset *tomorrow = RisetForObserver(&backup, index);
    if (tomorrow == nil) {
        return nil;
    }

    if ([today.peak.time timeIntervalSinceDate: today.rise.time] < 0) {
        today.peak = tomorrow.peak;
        today.set = tomorrow.set;
    } else {
        today.set = tomorrow.set;
    }
    return today;
}

+ (void)getRisetInLocation:(double) longitude latitude: (double) latitude altitude: (double)altitude forTime: (NSDate *) time completion:(void (^)(AstroRiset *sun, AstroRiset *moon))handler {
    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, time, &now);

    AstroRiset *sunriset = ModifiedRisetForObserver(&now, SUN);
    AstroRiset *moonriset = ModifiedRisetForObserver(&now, MOON);

    if (handler) {
        handler(sunriset, moonriset);
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
    double time = ModifiedJulianDate(d);
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

+ (NSArray *)getRiseSetForAllSolarSystemObjectsInLongitude:(double) longitude latitude: (double) latitude altitude: (double)altitude forTime: (NSDate *) time {

    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, time, &now);

    NSMutableArray *array = [NSMutableArray array];
    for (int i = MERCURY; i <= MOON; i++) {
        AstroRiset *riset = ModifiedRisetForObserver(&now, i);
        if (riset != nil) {
            [array addObject:riset];
        }
    }
    return array;
}

void ConfigureObserver(double longitude, double latitude, double altitude, NSDate *time, Now *obj)
{
    memset(obj, 0, sizeof(Now));
    obj->n_lng = radian(longitude);
    obj->n_lat = radian(latitude);
    obj->n_elev = altitude / ERAD;
    obj->n_dip = 0;
    obj->n_temp = 15.0;
    obj->n_tz = 0;
    /* Construct the Julian Date */
    obj->n_mjd = ModifiedJulianDate(time);
    obj->n_pressure = 1010;
}

+ (SatelliteRiseSet *)getRiseSetForSatelliteWithTLE:(SatelliteTLE *)tle longitude: (double)longitude latitude: (double) latitude altitude: (double)altitude forTime: (NSDate *) time {
    using namespace std;
    /* Construct the TLE */
    Obj satillite, satillite_backup;
    /* Construct the Satellite */
    db_tle((char *)[tle.line0 UTF8String], (char *)[tle.line1 UTF8String], (char *)[tle.line2 UTF8String], &satillite);
    memcpy(&satillite_backup, &satillite, sizeof(Obj));

    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, latitude, time, &now);

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
        memcpy(&satillite, &satillite_backup, sizeof(Obj));
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
                sunNow.n_mjd = ModifiedJulianDate(peak.time);
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
