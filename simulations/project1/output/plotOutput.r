setwd("C:/Users/taubert/Desktop/grassDT-model-GitHub/uc-grassland-model/simulations/project1/output/")
data = read.delim("output_51.340199_12.360103_2013_2022_parameters_generic_v1.txt",skip=1, header=T,dec=".",sep="\t")

specComp = array(0,c(0,4))
for (i in unique(data[,1])) {
  index = which(data[,1]==i)
  specComp = rbind(specComp,c(data[index[1],1],data[index,3]))
}

par(mar = c(4.5,4.5,0.5,0.5))
plot(specComp[,1],specComp[,2],type="l",col="darkgreen",lwd=3,log="", xlab="Time (day)",ylab="PFT-Fraction [%]",ylim=c(0,max(specComp[,2:4])))
lines(specComp[,1],specComp[,3],type="l",col="darkblue",lwd=3)
lines(specComp[,1],specComp[,4],type="l",col="darkorange",lwd=3)
lines(specComp[,1],(specComp[,2]+specComp[,3]+specComp[,4]),type="l",col="black",lwd=3)
legend("bottomleft",c("PFT1","PFT2","PFT3"),col=c("darkgreen","darkblue","darkorange"),lty=1,lwd=3,pch=-1,cex=0.8)