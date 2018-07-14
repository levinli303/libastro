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

@interface AstroRiseSet : NSObject
@property (nonatomic, copy) NSDate * _Nullable rise;
@property (nonatomic, copy) NSDate * _Nullable set;
@property (nonatomic, copy) NSString * _Nonnull name;
@end

@interface SatellitePosition : NSObject
@property (nonatomic, copy) NSDate * _Nonnull time;
@property (nonatomic) double azimuth;
@property (nonatomic) double elevation;
@property (nonatomic) double range;
@end

@interface SatelliteRiseSet : NSObject
@property (nonatomic, copy) SatellitePosition * _Nullable rise;
@property (nonatomic, copy) SatellitePosition * _Nullable set;
@property (nonatomic, copy) SatellitePosition * _Nullable peak;
@property (nonatomic, copy) SatellitePosition * _Nonnull current;
@property (nonatomic, copy) NSString * _Nonnull name;
@end

@interface Astro : NSObject

+ (void)getMoonRiseSetWithLongitude: (double) longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time completion:(nullable void (^)(NSDate *_Nullable, NSDate *_Nullable, NSDate *_Nullable, NSDate *_Nullable))handler;
+ (NSArray<AstroRiseSet *>*_Nonnull)getRiseSetForAllSolarSystemObjectsInLongitude:(double) longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time;
+ (SatelliteRiseSet *_Nullable)getRiseSetForSatelliteWithTLE:(NSArray<NSString *>*_Nonnull)tle longitude: (double)longitude latitude: (double) latitude forTime: (NSDate *_Nonnull) time;

@end
