//
//  Astro.h
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AstroPosition : NSObject
@property (nonatomic, readonly) double elevation;
@property (nonatomic, readonly) double azimuth;
@property (nonatomic, readonly) NSDate *time;
@end

@interface AstroRiset : NSObject
@property (nonatomic, readonly) AstroPosition *rise;
@property (nonatomic, readonly) AstroPosition *peak;
@property (nonatomic, readonly) AstroPosition *set;
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

@interface Astro : NSObject

@property (class, nonatomic, readonly) LunarPhase *currentMoonPhase;

+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time completion:(nullable void (^)(AstroRiset * _Nullable sun, AstroRiset * _Nullable moon))handler;
+ (NSArray<AstroRiset *>*)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time;
+ (SatelliteRiseSet *)risetForSatelliteWithTLE:(SatelliteTLE *)tle longitude:(double)longitude latitude: (double)latitude altitude:(double)altitude forTime: (NSDate *)time;

@end

NS_ASSUME_NONNULL_END
