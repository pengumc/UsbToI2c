    def up(self, leg, amount=1, direction=1.0):
        B = leg * 3 + 1
        C = leg * 3 + 2
        #up beta
        if leg%2:
            self._buffer[B] += int(amount*direction)
        else:
            self._buffer[B] -= int(amount*direction)
        #calculate angle beta
        beta = (self._buffer[B] - 72) * self._K
        #print("beta = " + str(beta))
        #angle gamma
        gamma = (72 - self._buffer[C]) * self._K - (pi/2)
        #print("gamma = " + str(gamma))
        #calculate x
        posx = math.cos(beta) * self._lengths[B]
        + math.cos(beta + gamma) * self._lengths[C]
        #print("x = " + str(posx))
        #calc required gamma
        gammareq = -math.acos( (posx - self._lengths[B] * math.cos(beta)) /
                              self._lengths[C] ) - beta
        #print("gammareq = " +str(gammareq))
        #gammareq -> servo pos
        self._buffer[C] = int(72-((gammareq +pi/2)/ self._K))
        print("new C = " + str(self._buffer[C]))

    def old_forward(self, leg=0, amount=1.0):
        A = leg * 3
        B = leg * 3 + 1
        C = leg * 3 + 2
        #get angle alpha
        alpha = (72 - self._buffer[A]) * self._K
        #print("alpha = " +str(alpha))
        #get beta
        beta = (self._buffer[B] - 72) * self._K
        #print("beta = " + str(beta))
        #get gamma
        gamma = (72 - self._buffer[C]) * self._K - (pi/2)
        #print("gamma = " + str(gamma))
        #calculate x
        posx=  ( self._lengths[A] + math.cos(beta) * self._lengths[B]
                + math.cos(beta + gamma) * self._lengths[C])* math.cos(alpha)
        #print("x = " + str(posx))
        #forward alpha
        if leg%2:
            self._buffer[A] += int(amount)
        else:
            self._buffer[A] -= int(amount)
        #get new alpha
        alpha = (72 - self._buffer[A]) * self._K
        #print("new alpha = " +str(alpha))
        #calculate x again
        newposx=  ( self._lengths[A] + math.cos(beta) * self._lengths[B]
                + math.cos(beta + gamma) * self._lengths[C])* math.cos(alpha)
        #print("x = " + str(newposx))
        diff = posx - newposx
        print("diff = " + str(diff))
        gammareq = -math.acos(diff / self._lengths[C]) - beta
        #gammareq -> servo pos
        gammatic = 72 - ((gammareq + pi/2)/ self._K)
        print gammatic
        self._buffer[C] = int(gammatic)
        print("new C = " + str(self._buffer[C]))

    def forward(self, leg=0, amount=1.0):
        A = leg * 3 
        B = leg * 3 + 1
        C = leg * 3 + 2
        #get angle alpha
        alpha = -(72 - self._buffer[A]) * self._K
        print("a = " +str(alpha))
        #get beta
        beta = (self._buffer[B] - 72) * self._K
        print("b = " + str(beta))
        #get gamma
        gamma = (72 - self._buffer[C]) * self._K - (pi/2)
        print("c = " + str(gamma))
        #calculate x
        posx=  ( self._lengths[A] + math.cos(beta) * self._lengths[B]
                + math.cos(beta + gamma) * self._lengths[C])* math.cos(alpha)
        print("x = " + str(posx))
        #update alpha
        if leg%2:
            self._buffer[A] += int(amount)
        else:
            self._buffer[A] -= int(amount)

        #get angle alpha
        alpha = (72 - self._buffer[A]) * self._K
        #calc gammareq
        gammareq = -math.acos( posx / (self._lengths[C] * math.cos(alpha))
                              - self._lengths[A] / self._lengths[C]
                              - math.cos(beta) * self._lengths[B] /
                              self._lengths[C]) - beta
        print("c = " + str(gammareq))
        temp = (72 - ((gammareq + pi/2)/ self._K))
        print("t = " + str(temp))
        self._buffer[C] = int(temp)
        self.send_pos()
