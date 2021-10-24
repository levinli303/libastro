//
//  Astro.h
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_CLOSED_ENUM(NSUInteger, StarRisetStatus) {
    StarRisetStatusNone,
    StarRisetStatusNeverRise,
    StarRisetStatusNeverSet,
};

@interface AstroPosition : NSObject
@property (nonatomic, readonly) double elevation;
@property (nonatomic, readonly) double azimuth;
@property (nonatomic, readonly) NSDate *time;
@end

@interface AstroRiset : NSObject
@property (nullable, nonatomic, readonly) AstroPosition *rise;
@property (nullable, nonatomic, readonly) AstroPosition *peak;
@property (nullable, nonatomic, readonly) AstroPosition *set;
@property (nonatomic, readonly) AstroPosition *current;
@property (nonatomic, readonly) NSString *name;
@end

@interface SatelliteTLE : NSObject
@property (nonatomic, readonly) NSString *line0;
@property (nonatomic, readonly) NSString *line1;
@property (nonatomic, readonly) NSString *line2;
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 line2: (NSString *)line2;
@end

@interface SatelliteStatus : NSObject
@property (readonly) double subLongitude;
@property (readonly) double subLatitude;
@property (readonly) double elevation;
@end

@interface SatellitePosition : NSObject
@property (nonatomic, readonly) NSDate *time;
@property (nonatomic, readonly) double azimuth;
@property (nonatomic, readonly) double elevation;
@property (nonatomic, readonly) double range;
@end

@interface SatelliteRiseSet : NSObject
@property (nonatomic, readonly, nullable) SatellitePosition *rise;
@property (nonatomic, readonly, nullable) SatellitePosition *set;
@property (nonatomic, readonly, nullable) SatellitePosition *peak;
@property (nonatomic, readonly) SatellitePosition *current;
@property (nonatomic, readonly) NSString *name;
@end

@interface LunarPhase : NSObject
@property (nonatomic, readonly) NSDate *nextNew;
@property (nonatomic, readonly) NSDate *nextFull;
@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly) double phase;
@property (nonatomic, readonly) double normalizedPhase;
@property (nonatomic, readonly) BOOL isFirstHalf;
@end

@interface SatellitePass : NSObject
@property (nonatomic, readonly) AstroPosition *rise;
@property (nonatomic, readonly) AstroPosition *peak;
@property (nonatomic, readonly) AstroPosition *set;
@property (nonatomic, nullable, readonly) AstroPosition *visiblePeak;
@property (nonatomic, nullable, readonly) AstroPosition *visibleRise;
@property (nonatomic, nullable, readonly) AstroPosition *visibleSet;
@end

@interface StarRiset : NSObject
@property (nullable, nonatomic, readonly) AstroPosition *rise;
@property (nullable, nonatomic, readonly) AstroPosition *peak;
@property (nullable, nonatomic, readonly) AstroPosition *set;
@property (nonatomic, readonly) AstroPosition *current;
@end

@interface Astro : NSObject

@property (class, nonatomic, readonly) LunarPhase *currentMoonPhase;

+ (LunarPhase *)moonPhaseAtTime:(NSDate *)time;
+ (AstroRiset *)objectRisetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time objectIndex:(NSInteger)index;
+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time completion:(nullable void (^)(AstroRiset *sun, AstroRiset * moon))handler;
+ (NSArray<AstroRiset *>*)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time;
+ (SatelliteRiseSet *)risetForSatelliteWithTLE:(SatelliteTLE *)tle longitude:(double)longitude latitude: (double)latitude altitude:(double)altitude forTime: (NSDate *)time;
+ (StarRiset *)risetForStarWithRA:(double)ra dec:(double)dec longitude:(double)longitude latitude:(double)latitude time:(NSDate *)time;
+ (double)getLSTInLocation:(double)longitude time:(NSDate *)time;
+ (nullable NSDate *)getSunAlt:(double)longitude latitude:(double)latitude altitude:(double)altitude time:(NSDate *)time goDown:(BOOL)goDown x:(double)x;
+ (nullable SatelliteStatus *)getSatelliteStatus:(SatelliteTLE *)tle atTime:(NSDate *)time;
+ (nullable SatellitePass *)getSatelliteNextRiset:(SatelliteTLE *)tle atTime:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude;

@end

NS_ASSUME_NONNULL_END
