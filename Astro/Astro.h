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
@property (nonatomic, readonly) StarRisetStatus status;
@property (nullable, nonatomic, readonly) AstroPosition *rise;
@property (nullable, nonatomic, readonly) AstroPosition *peak;
@property (nullable, nonatomic, readonly) AstroPosition *set;
@property (nonatomic, readonly) AstroPosition *current;
@property (nonatomic, readonly) NSString *name;
@end

@interface SatelliteTLE : NSObject
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 Line2: (NSString *)line2;
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
@end

@interface StarRiset : NSObject
@property (nullable, nonatomic, readonly) AstroPosition *rise;
@property (nullable, nonatomic, readonly) AstroPosition *peak;
@property (nullable, nonatomic, readonly) AstroPosition *set;
@property (nonatomic, readonly) AstroPosition *current;
@end

@interface Astro : NSObject

@property (class, nonatomic, readonly) LunarPhase *currentMoonPhase;

+ (AstroRiset *)objectRisetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time objectIndex:(NSInteger)index;
+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time completion:(nullable void (^)(AstroRiset *sun, AstroRiset * moon))handler;
+ (NSArray<AstroRiset *>*)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time;
+ (SatelliteRiseSet *)risetForSatelliteWithTLE:(SatelliteTLE *)tle longitude:(double)longitude latitude: (double)latitude altitude:(double)altitude forTime: (NSDate *)time;
+ (StarRiset *)risetForStarWithRA:(double)ra dec:(double)dec longitude:(double)longitude latitude:(double)latitude time:(NSDate *)time;
+ (double)getLSTInLocation:(double)longitude time:(NSDate *)time;

@end

NS_ASSUME_NONNULL_END
