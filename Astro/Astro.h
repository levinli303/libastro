//
//  Astro.h
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AstroPosition : NSObject <NSCopying>
@property (nonatomic) double elevation;
@property (nonatomic) double azimuth;
@property (nonatomic, copy) NSDate *time;
@end

@interface AstroRiset : NSObject
@property (nonatomic) AstroPosition *rise;
@property (nonatomic) AstroPosition *peak;
@property (nonatomic) AstroPosition *set;
@property (nonatomic, copy) NSString *name;
@end

@interface SatelliteTLE : NSObject
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 Line2: (NSString *)line2;
@end

@interface SatellitePosition : NSObject
@property (nonatomic, copy) NSDate *time;
@property (nonatomic) double azimuth;
@property (nonatomic) double elevation;
@property (nonatomic) double range;
@end

@interface SatelliteRiseSet : NSObject
@property (nonatomic, nullable) SatellitePosition *rise;
@property (nonatomic, nullable) SatellitePosition *set;
@property (nonatomic, nullable) SatellitePosition *peak;
@property (nonatomic) SatellitePosition *current;
@property (nonatomic, copy) NSString *name;
@end

@interface LunarPhase : NSObject
@property (nonatomic, copy) NSDate *nextNew;
@property (nonatomic, copy) NSDate *nextFull;
@property (nonatomic, copy) NSString *name;
@property (nonatomic) double phase;
@end

@interface Astro : NSObject

@property (class, nonatomic, readonly) LunarPhase *currentMoonPhase;

+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time completion:(nullable void (^)(AstroRiset * _Nullable sun, AstroRiset * _Nullable moon))handler;
+ (NSArray<AstroRiset *>*)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time;
+ (SatelliteRiseSet *)risetForSatelliteWithTLE:(SatelliteTLE *)tle longitude:(double)longitude latitude: (double)latitude altitude:(double)altitude forTime: (NSDate *)time;

@end

NS_ASSUME_NONNULL_END
