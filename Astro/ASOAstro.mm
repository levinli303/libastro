//
//  ASOAstro.m
//  ASOAstro
//
//  Created by Levin Li on 2017/7/25.
//  Copyright © 2017 Astroweather. All rights reserved.
//

#import "ASOAstro.h"
#include "astro_common.h"

#undef lat

NSDate *ModernDate(double d)
{
    return [NSDate dateWithTimeIntervalSince1970:EphemToEpochTime(d)];
}

double ModifiedJulianDate(NSDate *time)
{
    return EpochToEphemTime([time timeIntervalSince1970]);
}

@implementation ASOAstroPosition
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

@implementation ASOAstroRiset
- (instancetype)initWithRise:(ASOAstroPosition *)rise peak:(ASOAstroPosition *)peak set:(ASOAstroPosition *)set current:(ASOAstroPosition *)current name:(NSString *)name {
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

@implementation ASOSatelliteTLE : NSObject
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 line2: (NSString *)line2 {
    self = [super init];
    if (self) {
        _line0 = line0;
        _line1 = line1;
        _line2 = line2;
    }
    return self;
}
@end

@implementation ASOSatelliteStatus : NSObject
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

@implementation ASOLunarPhase
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

@implementation ASOSatellitePass

- (instancetype)initWithRise:(ASOAstroPosition *)rise set:(ASOAstroPosition *)set peak:(ASOAstroPosition *)peak visibleRise:(nullable ASOAstroPosition *)visibleRise  visibleSet:(nullable ASOAstroPosition *)visibleSet {
    self = [super init];
    if (self) {
        _rise = rise;
        _set = set;
        _peak = peak;
        _visibleRise = visibleRise;
        _visibleSet = visibleSet;
        if (visibleRise && visibleSet) {
            if ([[peak time] timeIntervalSince1970] < [[visibleRise time] timeIntervalSince1970] || [[peak time] timeIntervalSince1970] > [[visibleSet time] timeIntervalSince1970]) {
                _visiblePeak = [visibleRise elevation] > [visibleSet elevation] ? visibleRise : visibleSet;
            } else  {
                _visiblePeak = peak;
            }
        } else {
            _visiblePeak = nil;
        }
    }
    return self;
}

@end

@implementation ASOStarRiset

- (instancetype)initWithRise:(ASOAstroPosition *)rise set:(ASOAstroPosition *)set peak:(ASOAstroPosition *)peak current:(ASOAstroPosition *)current {
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

@implementation ASOSunTime

- (instancetype)initWithStartTime:(NSTimeInterval)startTime endTime:(NSTimeInterval)endTime state:(ASOSunState)state {
    self = [super init];
    if (self) {
        _state = state;
        _startTime = startTime;
        _endTIme = endTime;
    }
    return self;
}
@end

@implementation ASOAstro

+ (ASOAstroRiset *)objectRisetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time objectIndex:(NSInteger)index up:(BOOL)up {
    /* Construct the observer */
    Now now;
    ConfigureObserver(longitude, latitude, altitude, [time timeIntervalSince1970], &now);

    RiseSet riset;
    NSString *name = [NSString stringWithUTF8String:GetStarName((int)index)];
    double el, az;
    int result = GetModifiedRiset(&now, (int)index, &riset, &el, &az, (bool)up);
    ASOAstroPosition *current = [[ASOAstroPosition alloc] initWithAzimuth:az elevation:el time:time];
    ASOAstroPosition *rise = nil;
    ASOAstroPosition *set = nil;
    ASOAstroPosition *peak = nil;
    if (result == 0) {
        rise = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_riseaz elevation:0 time:ModernDate(riset.rs_risetm)];

        set = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_setaz elevation:0 time:ModernDate(riset.rs_settm)];

        peak = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_tranaz elevation:riset.rs_tranalt time:ModernDate(riset.rs_trantm)];
    }
    return [[ASOAstroRiset alloc] initWithRise:rise peak:peak set:set current:current name:name];
}

+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time completion:(void (^)(ASOAstroRiset *sun, ASOAstroRiset *moon))handler {
    ASOAstroRiset *sunriset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:SUN up:YES];
    ASOAstroRiset *moonriset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:MOON up:YES];

    if (handler)
        handler(sunriset, moonriset);
}

+ (ASOLunarPhase *)currentMoonPhase {
    return [self moonPhaseAtTime:[NSDate date]];
}

+ (ASOLunarPhase *)moonPhaseAtTime:(NSDate *)time {
    NSDate *prevNew =  [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * -2, 0)];
    NSDate *nextNew = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * 2, 0)];
    NSDate *nextFull = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([time timeIntervalSince1970], M_PI * 2, M_PI)];
    NSDate *prevNextFull = [NSDate dateWithTimeIntervalSince1970:FindMoonPhase([prevNew timeIntervalSince1970], M_PI * 2, M_PI)];
    double phase = CurrentMoonPhase([time timeIntervalSince1970]);

    return [[ASOLunarPhase alloc] initWithPhase:phase isFirstHalf:[time timeIntervalSinceDate:prevNextFull] <= 0 nextNewMoon:nextNew nextFullMoon:nextFull];
}

+ (NSArray *)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double) latitude altitude:(double)altitude forTime:(NSDate *)time up:(BOOL)up {
    NSMutableArray *array = [NSMutableArray array];
    for (int i = MERCURY; i <= MOON; i++) {
        ASOAstroRiset *riset = [self objectRisetInLocation:longitude latitude:latitude altitude:altitude forTime:time objectIndex:i up:up];
        [array addObject:riset];
    }
    return array;
}

+ (ASOStarRiset *)risetForStarWithRA:(double)ra dec:(double)dec longitude:(double)longitude latitude:(double)latitude time:(NSDate *)time {
    double riseTime, setTime, transitTime;
    int status;
    double az_r, az_s, az_c, az_t, el_c, el_t;
    GetRADECRiset(ra, dec, longitude, latitude, [time timeIntervalSince1970], &riseTime, &setTime, &transitTime, &status, &az_r, &az_s, &az_c, &az_t, &el_c, &el_t);
    ASOAstroPosition *current = [[ASOAstroPosition alloc] initWithAzimuth:az_c elevation:el_c time:time];
    ASOAstroPosition *rise = nil;
    ASOAstroPosition *set = nil;
    ASOAstroPosition *peak = nil;
    if (status == 0)
    {
        rise = [[ASOAstroPosition alloc] initWithAzimuth:az_r elevation:0 time:[NSDate dateWithTimeIntervalSince1970:riseTime]];
        set = [[ASOAstroPosition alloc] initWithAzimuth:az_s elevation:0 time:[NSDate dateWithTimeIntervalSince1970:setTime]];
        peak = [[ASOAstroPosition alloc] initWithAzimuth:az_t elevation:el_t time:[NSDate dateWithTimeIntervalSince1970:transitTime]];
    }

    return [[ASOStarRiset alloc] initWithRise:rise set:set peak:peak current:current];
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

+ (nullable ASOSatelliteStatus *)getSatelliteStatus:(ASOSatelliteTLE *)tle atTime:(NSDate *)time {
    double sublng, sublat, elevation;
    int result = GetSatelliteStatus([[tle line0] UTF8String], [[tle line1] UTF8String], [[tle line2] UTF8String], [time timeIntervalSince1970], &sublng, &sublat, &elevation);

    if (result) {
        return [[ASOSatelliteStatus alloc] initWithSubLongitude:sublng subLatitude:sublat elevation:elevation];
    }
    return nil;
}

+ (nullable ASOSatellitePass *)getSatelliteNextRiset:(ASOSatelliteTLE *)tle atTime:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude {
    RiseSet riset;
    RiseSet visibleRiset;
    double visibleRiseAlt, visibleSetAlt;
    int result = GetNextSatellitePass([[tle line0] UTF8String], [[tle line1] UTF8String], [[tle line2] UTF8String], [time timeIntervalSince1970], longitude, latitude, altitude, &riset, &visibleRiset, &visibleRiseAlt, &visibleSetAlt);
    if (result != 0)
        return nil;

    ASOAstroPosition *rise = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_riseaz elevation:0 time:ModernDate(riset.rs_risetm)];
    ASOAstroPosition *set = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_setaz elevation:0 time:ModernDate(riset.rs_settm)];
    ASOAstroPosition *peak = [[ASOAstroPosition alloc] initWithAzimuth:riset.rs_tranaz elevation:riset.rs_tranalt time:ModernDate(riset.rs_trantm)];

    ASOAstroPosition *visibleRise = nil;
    ASOAstroPosition *visibleSet = nil;

    if (visibleRiset.rs_flags == 0)
    {
        visibleRise = [[ASOAstroPosition alloc] initWithAzimuth:visibleRiset.rs_riseaz elevation:visibleRiseAlt time:ModernDate(visibleRiset.rs_risetm)];
        visibleSet = [[ASOAstroPosition alloc] initWithAzimuth:visibleRiset.rs_setaz elevation:visibleSetAlt time:ModernDate(visibleRiset.rs_settm)];
    }
    return [[ASOSatellitePass alloc] initWithRise:rise set:set peak:peak visibleRise:visibleRise visibleSet:visibleSet];
}

+ (ASOAstroPosition *)getStarPosition:(double)ra dec:(double)dec raPm:(double)raPm decPm:(double)decPM time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude {
    Now now;
    ConfigureObserver(longitude, latitude, altitude, [time timeIntervalSince1970], &now);

    Obj obj;
    obj.o_type = FIXED;
    obj.f_RA = (float)radian(ra);
    obj.f_dec = (float)radian(dec);
    obj.f_epoch = J2000;
    obj.f_pmRA = (float)(raPm / 1000 / 3600 / 180 * M_PI / 365.25);
    obj.f_pmdec = (float)(decPM / 1000 / 3600 / 180 * M_PI / 365.25);
    obj_cir(&now, &obj);

    return [[ASOAstroPosition alloc] initWithAzimuth:(double)obj.f.co_az elevation:(double)obj.f.co_alt time:time];
}

+ (ASOAstroPosition *)getSolarSystemObjectPosition:(NSInteger)index time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude {
    Now now;
    ConfigureObserver(longitude, latitude, altitude, [time timeIntervalSince1970], &now);

    Obj *objs;
    getBuiltInObjs(&objs);

    Obj obj = objs[index];
    obj_cir(&now, &obj);
    return [[ASOAstroPosition alloc] initWithAzimuth:(double)obj.any.co_az elevation:(double)obj.any.co_alt time:time];
}

+ (ASOAstroPosition *)getSatellitePosition:(ASOSatelliteTLE *)tle time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude {
    double el, az;
    int result = GetSatellitePosition([[tle line0] UTF8String], [[tle line1] UTF8String], [[tle line2] UTF8String], longitude, latitude, altitude, [time timeIntervalSince1970], &el, &az);

    if (result) {
        return [[ASOAstroPosition alloc] initWithAzimuth:az elevation:el time:time];
    }
    return nil;
}

+ (NSArray<ASOSunTime *> *)getSunTimes:(NSDate *)startTime endTime:(NSDate *)endTime longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude {
    NSMutableArray *array = [NSMutableArray array];
    auto times = GetSunDetails(longitude, latitude, altitude, [startTime timeIntervalSince1970], [endTime timeIntervalSince1970]);

    for (auto sunPeriod : times)
    {
        [array addObject:[[ASOSunTime alloc] initWithStartTime:sunPeriod.start endTime:sunPeriod.end state:(ASOSunState)sunPeriod.status]];
    }

    return [array copy];
}

@end
