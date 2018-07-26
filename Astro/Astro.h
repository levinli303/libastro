//
//  Astro.h
//  Astro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, AstroPlanet) {
    AstroPlanetSun,
    AstroPlanetMoon,
    AstroPlanetMercury,
    AstroPlanetVenus,
    AstroPlanetMars,
    AstroPlanetJupiter,
    AstroPlanetSaturn,
    AstroPlanetUranus,
    AstroPlanetNeptune,
    AstroPlanetPluto,
};

@interface AstroPosition : NSObject <NSCopying>
@property (nonatomic) double elevation;
@property (nonatomic) double azimuth;
@property (nonatomic, copy) NSDate * _Nonnull time;
@end

@interface AstroRiset : NSObject
@property (nonatomic) AstroPosition * _Nonnull rise;
@property (nonatomic) AstroPosition * _Nonnull peak;
@property (nonatomic) AstroPosition * _Nonnull set;
@end

@interface AstroRiseSet : NSObject
@property (nonatomic, copy) NSDate * _Nullable rise;
@property (nonatomic, copy) NSDate * _Nullable set;
@property (nonatomic, copy) NSString * _Nonnull name;
@end

@interface SatelliteTLE : NSObject
- (instancetype _Nonnull)initWithLine0: (NSString *_Nonnull)line0 line1: (NSString *_Nonnull)line1 Line2: (NSString *_Nonnull)line2;
@end

@interface SatellitePosition : NSObject
@property (nonatomic, copy) NSDate * _Nonnull time;
@property (nonatomic) double azimuth;
@property (nonatomic) double elevation;
@property (nonatomic) double range;
@end

@interface SatelliteRiseSet : NSObject
@property (nonatomic) SatellitePosition * _Nullable rise;
@property (nonatomic) SatellitePosition * _Nullable set;
@property (nonatomic) SatellitePosition * _Nullable peak;
@property (nonatomic) SatellitePosition * _Nonnull current;
@property (nonatomic, copy) NSString * _Nonnull name;
@end

@interface LunarPhase : NSObject
@property (nonatomic, copy) NSDate * _Nonnull nextNew;
@property (nonatomic, copy) NSDate * _Nonnull nextFull;
@property (nonatomic, copy) NSString * _Nonnull name;
@property (nonatomic) double phase;
@end

@interface Astro : NSObject

+ (void)getMoonRiseSetWithLongitude:(double) longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time completion:(nullable void (^)(NSDate *_Nullable, NSDate *_Nullable, NSDate *_Nullable, NSDate *_Nullable, LunarPhase *_Nonnull))handler;
+ (void)getRisetInLocation:(double) longitude latitude: (double) latitude altitude: (double)altitude forTime: (NSDate *_Nonnull) time completion:(nullable void (^)(AstroRiset *_Nullable sun, AstroRiset *_Nullable moon))handler;
+ (LunarPhase *_Nonnull)getCurrentMoonPhase;
+ (NSArray<AstroRiseSet *>*_Nonnull)getRiseSetForAllSolarSystemObjectsInLongitude:(double) longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time;
+ (SatelliteRiseSet *_Nonnull)getRiseSetForSatelliteWithTLE:(SatelliteTLE *_Nonnull)tle longitude: (double)longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time;

@end
