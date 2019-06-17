/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "FIRInstallationsSingleOperationPromiseCache.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

@interface FIRInstallationsSingleOperationPromiseCache <ResultType>()
@property(nonatomic, readonly) FBLPromise *_Nonnull (^promiseFactory)(void);
@property(atomic, nullable) FBLPromise *pendingPromise;
@end

@implementation FIRInstallationsSingleOperationPromiseCache

- (instancetype)initWithPromiseFactory:(FBLPromise<id> *_Nonnull (^)(void))factory {
  if (factory == nil) {
    [NSException raise:NSInvalidArgumentException format:@"`factory` must not be `nil`."];
  }

  self = [super init];
  if (self) {
    _promiseFactory = [factory copy];
  }
  return self;
}

- (FBLPromise *)getExistingPendingOrCreateNewPromise {
  if (!self.pendingPromise) {
    self.pendingPromise = self.promiseFactory();

    self.pendingPromise
        .then(^id(id result) {
          self.pendingPromise = nil;
          return nil;
        })
        .catch(^void(NSError *error) {
          self.pendingPromise = nil;
        });
  }

  return self.pendingPromise;
}

@end