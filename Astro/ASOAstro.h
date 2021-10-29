//
//  ASOAstro.h
//  ASOAstro
//
//  Created by 李林峰 on 2017/7/25.
//  Copyright © 2017年 Bilgisayar. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(AstroPosition)
@interface ASOAstroPosition : NSObject
@property (nonatomic, readonly) double elevation;
@property (nonatomic, readonly) double azimuth;
@property (nonatomic, readonly) NSDate *time;
@end

NS_SWIFT_NAME(AstroRiset)
@interface ASOAstroRiset : NSObject
@property (nullable, nonatomic, readonly) ASOAstroPosition *rise;
@property (nullable, nonatomic, readonly) ASOAstroPosition *peak;
@property (nullable, nonatomic, readonly) ASOAstroPosition *set;
@property (nonatomic, readonly) ASOAstroPosition *current;
@property (nonatomic, readonly) NSString *name;
@end

NS_SWIFT_NAME(SatelliteTLE)
@interface ASOSatelliteTLE : NSObject
@property (nonatomic, readonly) NSString *line0;
@property (nonatomic, readonly) NSString *line1;
@property (nonatomic, readonly) NSString *line2;
- (instancetype)initWithLine0: (NSString *)line0 line1: (NSString *)line1 line2: (NSString *)line2;
@end

NS_SWIFT_NAME(SatelliteStatus)
@interface ASOSatelliteStatus : NSObject
@property (readonly) double subLongitude;
@property (readonly) double subLatitude;
@property (readonly) double elevation;
@end

NS_SWIFT_NAME(LunarPhase)
@interface ASOLunarPhase : NSObject
@property (nonatomic, readonly) NSDate *nextNew;
@property (nonatomic, readonly) NSDate *nextFull;
@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly) double phase;
@property (nonatomic, readonly) double normalizedPhase;
@property (nonatomic, readonly) BOOL isFirstHalf;
@end

NS_SWIFT_NAME(SatellitePass)
@interface ASOSatellitePass : NSObject
@property (nonatomic, readonly) ASOAstroPosition *rise;
@property (nonatomic, readonly) ASOAstroPosition *peak;
@property (nonatomic, readonly) ASOAstroPosition *set;
@property (nonatomic, nullable, readonly) ASOAstroPosition *visiblePeak;
@property (nonatomic, nullable, readonly) ASOAstroPosition *visibleRise;
@property (nonatomic, nullable, readonly) ASOAstroPosition *visibleSet;
@end

NS_SWIFT_NAME(StarRiset)
@interface ASOStarRiset : NSObject
@property (nullable, nonatomic, readonly) ASOAstroPosition *rise;
@property (nullable, nonatomic, readonly) ASOAstroPosition *peak;
@property (nullable, nonatomic, readonly) ASOAstroPosition *set;
@property (nonatomic, readonly) ASOAstroPosition *current;
@end

NS_SWIFT_NAME(Astro)
@interface ASOAstro : NSObject

@property (class, nonatomic, readonly) ASOLunarPhase *currentMoonPhase;

+ (ASOLunarPhase *)moonPhaseAtTime:(NSDate *)time;
+ (ASOAstroRiset *)objectRisetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time objectIndex:(NSInteger)index;
+ (void)risetInLocation:(double)longitude latitude:(double)latitude altitude:(double)altitude forTime:(NSDate *)time completion:(nullable void (^)(ASOAstroRiset *sun, ASOAstroRiset * moon))handler;
+ (NSArray<ASOAstroRiset *>*)risetForSolarSystemObjectsInLongitude:(double)longitude latitude:(double)latitude altitude: (double)altitude forTime:(NSDate *)time;
+ (ASOStarRiset *)risetForStarWithRA:(double)ra dec:(double)dec longitude:(double)longitude latitude:(double)latitude time:(NSDate *)time;
+ (double)getLSTInLocation:(double)longitude time:(NSDate *)time;
+ (nullable NSDate *)getSunAlt:(double)longitude latitude:(double)latitude altitude:(double)altitude time:(NSDate *)time goDown:(BOOL)goDown x:(double)x;
+ (nullable ASOSatelliteStatus *)getSatelliteStatus:(ASOSatelliteTLE *)tle atTime:(NSDate *)time;
+ (nullable ASOSatellitePass *)getSatelliteNextRiset:(ASOSatelliteTLE *)tle atTime:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude;
+ (ASOAstroPosition *)getStarPosition:(double)ra dec:(double)dec raPm:(double)raPm decPm:(double)decPM time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude;
+ (ASOAstroPosition *)getSolarSystemObjectPosition:(NSInteger)index time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude;
+ (nullable ASOAstroPosition *)getSatellitePosition:(ASOSatelliteTLE *)tle time:(NSDate *)time longitude:(double)longitude latitude:(double)latitude altitude:(double)altitude;

@end

NS_ASSUME_NONNULL_END
