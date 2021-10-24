//
//  Astro.m
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import "Astro.h"
#include "astro_common.h"

#undef lat

#define MIN_VISIBLE_ELEVATION_DEGREE        10
#define MIN_VISIBLE_ELEVATION_RADIAN        (radian(MIN_VISIBLE_ELEVATION_DEGREE))
#define MAX_FORECAST_DAY                    5

NSDate *ModernDate(double d)
{
    return [NSDate dateWithTimeIntervalSince1970:EphemToEpochTime(d)];
}

double ModifiedJulianDate(NSDate *time)
{
    return EpochToEphemTime([time timeIntervalSince1970]);
}

@implementation AstroPosition
- (instancetype)initWithAzimuth:(double)azimuth elevation:(double)elevation time:(NSDate *)time {
    self = [super init];
    if (self) {
        _azimuth = azimuth;
        _elevation = elevation;
        _time = time;
    }
    return self;
}
@end

@implementation AstroRiset
- (instancetype)initWithRise:(AstroPosition *)rise peak:(AstroPosition *)peak set:(AstroPosition *)set current:(AstroPosition *)current name:(NSString *)name {
    self = [super init];
    if (self) {
        _rise = rise;
        _peak = peak;
        _set = set;
        _current = current;
        _name = name;
    }
    return self;
}

@end

@interface SatelliteTLE ()
@property (nonatomic, readonly) NSString *line0;
@property (nonatomic, readonly) NSString *line1;
@property (nonatomic, readonly) NSString *line2;
@end

@implementation SatelliteTLE : NSObject
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 Line2: (NSString *)line2 {
    self = [super init];
    if (self) {
        _line0 = line0;
        _line1 = line1;
        _line2 = line2;
    }
    return self;
}
@end

@implementation SatelliteStatus : NSObject
- (instancetype)initWithSubLongitude:(double)subLongitude subLatitude:(double)subLatitude elevation:(double)elevation {
    self = [super init];
    if (self) {
        _subLongitude = subLongitude;
        _subLatitude = subLatitude;
        _elevation = elevation;
    }
    return self;
}
@end

@implementation SatellitePosition
- (instancetype)initWithTime:(NSDate *)time azimuth:(double)azimuth elevation:(double)elevation range:(double)range {
    self = [super init];
    if (self) {
        _time = time;
        _azimuth = azimuth;
        _elevation = elevation;
        _range = range;
    }
    return self;
}
@end

@implementation SatelliteRiseSet
- (instancetype)initWithName:(NSString *)name current:(SatellitePosition *)current rise: (SatellitePosition *)rise peak: (SatellitePosition *)peak set: (SatellitePosition *)set {
    self = [super init];
    if (self) {
        _name = name;
        _current = current;
        _rise = rise;
        _peak = peak;
        _set = set;
    }
    return self;
}
@end

@implementation LunarPhase
- (instancetype)initWithPhase:(double)phase isFirstHalf:(BOOL)isFirstHalf nextNewMoon:(NSDate *)nextNew nextFullMoon:(NSDate *)nextFull {
    self = [super init];
    if (self) {
        _nextFull = nextFull;
        _nextNew = nextNew;

        _phase = phase;
        _isFirstHalf = isFirstHalf;

        if (phase <= 0.01) {
            _name = @"New Moon";
        } else if (phase < 0.49) {
            if (isFirstHalf) {
                _name = @"Waxing Crescent";
            } else {
                _name = @"Waning Crescent";
            }
        } else if (phase <= 0.51) {
            if (isFirstHalf) {
                _name = @"First Quarter";
            } else {
                _name = @"Last Quarter";
            }
        } else if (phase < 0.99) {
            if (isFirstHalf) {
                _name = @"Waxing Gibbous";
            } else {
                _name = @"Waning Gibbous";
            }
        } else {
            _name = @"Full Moon";
        }
    }
    return self;
}

- (double)normalizedPhase {
    return _isFirstHalf ? (_phase * 0.5) : (1 - _phase * 0.5);
}
@end

@implementation StarRiset

- (instancetype)initWithRise:(AstroPosition *)rise set:(AstroPosition *)set peak:(AstroPosition *)peak current:(AstroPosition *)current {
    self = [super init];
    if (self) {
        _rise = rise;
        _set = set;
        _peak = peak;
        _current = current;
    }
    return self;
}

@end

@implementation Astro

+ (AstroRiset *)objectRisetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time objectIndex:(NSInteger)index {
    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, [time timeIntervalSince1970], &now);

    RiseSet riset;
    NSString *name = [NSString stringWithUTF8String:GetStarName((int)index)];
    double el, az;
    int result = GetModifiedRiset(&now, (int)index, &riset, &el, &az);
    AstroPosition *current = [[AstroPosition alloc] initWithAzimuth:az elevation:el time:time];
    AstroPosition *rise = nil;
    AstroPosition *set = nil;
    AstroPosition *peak = nil;
    if (result == 0) {
        rise = [[AstroPosition alloc] initWithAzimuth:riset.rs_riseaz elevation:0 time:ModernDate(riset.rs_risetm)];

        set = [[AstroPosition alloc] initWithAzimuth:riset.rs_setaz elevation:0 time:ModernDate(riset.rs_settm)];

        peak = [[AstroPosition alloc] initWithAzimuth:riset.rs_tranaz elevation:riset.rs_tranalt time:ModernDate(riset.rs_trantm)];
    }
    return [[AstroRiset alloc] initWithRise:rise peak:peak set:set current:current name:name];
}

+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time completion:(void (^)(AstroRiset *sun, AstroRiset *moon))handler {
    AstroRiset *sunriset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:SUN];
    AstroRiset *moonriset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:MOON];

    if (handler)
        handler(sunriset, moonriset);
}

+ (LunarPhase *)currentMoonPhase {
    return [self moonPhaseAtTime:[NSDate date]];
}

+ (LunarPhase *)moonPhaseAtTime:(NSDate *)time {
    NSDate *prevNew =  [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * -2, 0)];
    NSDate *nextNew = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * 2, 0)];
    NSDate *nextFull = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * 2, M_PI)];
    NSDate *prevNextFull = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([prevNew timeIntervalSince1970], M_PI * 2, M_PI)];
    double phase = CurrentMoonPhase([time timeIntervalSince1970]);

    return [[LunarPhase alloc] initWithPhase:phase isFirstHalf:[time timeIntervalSinceDate:prevNextFull] <= 0 nextNewMoon:nextNew nextFullMoon:nextFull];
}

+ (NSArray *)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double) latitude altitude:(double)altitude forTime:(NSDate *)time {
    NSMutableArray *array = [NSMutableArray array];
    for (int i = MERCURY; i <= MOON; i++) {
        AstroRiset *riset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:i];
        [array addObject:riset];
    }
    return array;
}

+ (SatelliteRiseSet *)risetForSatelliteWithTLE:(SatelliteTLE *)tle longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime: (NSDate *) time {
    /* Construct the TLE */
    Obj satillite, satillite_backup;
    /* Construct the Satellite */
    db_tle((char *)[tle.line0 UTF8String], (char *)[tle.line1 UTF8String], (char *)[tle.line2 UTF8String], &satillite);
    memcpy(&satillite_backup, &satillite, sizeof(Obj));

    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, latitude, [time timeIntervalSince1970], &now);

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

+ (StarRiset *)risetForStarWithRA:(double)ra dec:(double)dec longitude:(double)longitude latitude:(double)latitude time:(NSDate *)time {
    double riseTime, setTime, transitTime;
    int status;
    double az_r, az_s, az_c, az_t, el_c, el_t;
    GetRADECRiset(ra, dec, longitude, latitude, [time timeIntervalSince1970], &riseTime, &setTime, &transitTime, &status, &az_r, &az_s, &az_c, &az_t, &el_c, &el_t);
    AstroPosition *current = [[AstroPosition alloc] initWithAzimuth:az_c elevation:el_c time:time];
    AstroPosition *rise = nil;
    AstroPosition *set = nil;
    AstroPosition *peak = nil;
    if (status == 0)
    {
        rise = [[AstroPosition alloc] initWithAzimuth:az_r elevation:0 time:[NSDate dateWithTimeIntervalSince1970:riseTime]];
        set = [[AstroPosition alloc] initWithAzimuth:az_s elevation:0 time:[NSDate dateWithTimeIntervalSince1970:setTime]];
        peak = [[AstroPosition alloc] initWithAzimuth:az_t elevation:el_t time:[NSDate dateWithTimeIntervalSince1970:transitTime]];
    }

    return [[StarRiset alloc] initWithRise:rise set:set peak:peak current:current];
}

+ (double)getLSTInLocation:(double)longitude time:(NSDate *)time {
    return GetLST([time timeIntervalSince1970], longitude);
}

+ (NSDate *)getSunAlt:(double)longitude latitude:(double)latitude altitude:(double)altitude time:(NSDate *)time goDown:(BOOL)goDown x:(double)x
{
    const double step = 1.0 / 1440;
    const double limit = 2;
    Now now;
    ConfigureObserver(longitude, latitude, altitude, [time timeIntervalSince1970], &now);

    double jd = 0;
    if (FindAltXSun(&now, step, limit, 1, goDown ? 1 : 0, &jd, x))
        return nil;
    return ModernDate(jd);
}

+ (nullable SatelliteStatus *)getSatelliteStatus:(SatelliteTLE *)tle atTime:(NSDate *)time {
    double sublng, sublat, elevation;
    int result = GetSatelliteStatus([[tle line0] UTF8String], [[tle line1] UTF8String], [[tle line2] UTF8String], [time timeIntervalSince1970], &sublng, &sublat, &elevation);

    if (result) {
        return [[SatelliteStatus alloc] initWithSubLongitude:sublng subLatitude:sublat elevation:elevation];
    }
    return nil;
}

@end
